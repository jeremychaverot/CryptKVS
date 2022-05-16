/**
 * @file ckvs_client.c
 * @brief c.f. ckvs_client.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <json-c/json.h>
#include "ckvs_rpc.h"
#include "ckvs_utils.h"
#include "ckvs_io.h"
#include "ckvs.h"
#include "util.h"
#include "ckvs_crypto.h"
#include "ckvs_local.h"

#include <openssl/hmac.h>



/*
 * To avoid circular dependencies.
 */
//struct json_object;



int ckvs_client_stats(const char *url, int optargc, char **optargv) {
    if (optargc > 0) {
        //error
        return ERR_TOO_MANY_ARGUMENTS;
    }
    //check if the pointer is valid
    if (url == NULL) {
        //error
        return ERR_INVALID_ARGUMENT;
    }

    //initialiaze the struct ckvs_connection
    ckvs_connection_t conn;

    //initialiaze the struct ckvs
    struct CKVS ckvs;
    memset(&ckvs, 0, sizeof(struct CKVS));


    //initialize the connection and check errors
    int err = ckvs_rpc_init(&conn, (const char *) url);
    if (err != ERR_NONE) {
        //error
        return err;
    }

    err = ckvs_rpc(&conn, "stats");
    if (err != ERR_NONE) {
        //error
        return err;
    }

    struct json_object *root_obj = json_tokener_parse(conn.resp_buf);
    if (root_obj == NULL) {
        //error
        pps_printf("%s\n", "An error occured when parsing the string into a json object");
        ckvs_rpc_close(&conn);
        return ERR_IO;
    }


    err = retrieve_ckvs_from_json(&ckvs, root_obj);
    if (err != ERR_NONE) {
        //error
        ckvs_rpc_close(&conn);
        json_object_put(root_obj);
        return ERR_TIMEOUT;
    }

    //print the content downloaded from the server
    print_header(&(ckvs.header));

    //print the key of the entries
    for (size_t i = 0; i < ckvs.header.table_size; ++i) {
        if (strlen(ckvs.entries[i].key) != 0) {
            pps_printf("%s       : " STR_LENGTH_FMT(CKVS_MAXKEYLEN) "\n", "Key", ckvs.entries[i].key);
        }
    }

    //free the struct JSON
    err = json_object_put(root_obj);
    if (err != 1) {
        //error
        ckvs_rpc_close(&conn);
        return err;
    }

    //close the connection
    ckvs_rpc_close(&conn);

    //free pointers
    ckvs_close(&ckvs);

    return ERR_NONE;
}


int ckvs_client_get(const char *url, int optargc, char **optargv) {
    if (optargc < 2) return ERR_NOT_ENOUGH_ARGUMENTS;
    if (optargc > 2) return ERR_TOO_MANY_ARGUMENTS;

    const char *key = optargv[0];
    const char *pwd = optargv[1];
    //check if the pointeurs are valid
    if (key == NULL || pwd == NULL || url == NULL) {
        //error
        return ERR_INVALID_ARGUMENT;
    }

    //initialiaze the struct ckvs_connection
    ckvs_connection_t conn;

    //initialiaze the struct ckvs
    struct CKVS ckvs;
    memset(&ckvs, 0, sizeof(struct CKVS));


    //initialize the connection and check errors
    int err = ckvs_rpc_init(&conn, (const char *) url);
    if (err != ERR_NONE) {
        //error
        return err;
    }




    //initialize the struct ckvs_memrecord_t
    ckvs_memrecord_t ckvs_mem;
    memset(&ckvs_mem, 0, sizeof(ckvs_memrecord_t));

    //to generate in particular the auth_key and c1 and store them in ckvs_mem
    err = ckvs_client_encrypt_pwd(&ckvs_mem, key, pwd);
    if (err != ERR_NONE) {
        // error
        ckvs_rpc_close(&conn);
        ckvs_close(&ckvs);
        return err;
    }

    char* ready_key = NULL;
    CURL* curl = curl_easy_init();
    if (curl != NULL) {
        ready_key = curl_easy_escape(curl, key, strlen(key));
        if (ready_key == NULL) {
            //error
            ckvs_close(&ckvs);
            ckvs_rpc_close(&conn);
            curl_free(curl);
            return ERR_IO;
        }
        curl_easy_cleanup(curl);
    } else {
        //error
        ckvs_close(&ckvs);
        ckvs_rpc_close(&conn);
        return ERR_IO;
    }

    char buffer[SHA256_PRINTED_STRLEN];
    SHA256_to_string(&ckvs_mem.auth_key, buffer);

    //build the string containing the whole information
    char *page = calloc(SHA256_PRINTED_STRLEN + strlen(ready_key) + 19, sizeof(char));
    if (page == NULL) {
        ckvs_rpc_close(&conn);
        ckvs_close(&ckvs);
        curl_free(ready_key);
        return ERR_OUT_OF_MEMORY;
    }
    strcpy(page, "get?key=");
    strcat(page, ready_key);
    strcat(page, "&auth_key=");
    strncat(page, &buffer, SHA256_PRINTED_STRLEN);

    curl_free(ready_key);
    err = ckvs_rpc(&conn, page);
    free(page);
    if (err != ERR_NONE) {
        //error
        ckvs_rpc_close(&conn);
        ckvs_close(&ckvs);
        return err;
    }




    char* c2_str[SHA256_PRINTED_STRLEN + 1];
    ckvs_sha_t* c2= calloc(1, sizeof(ckvs_sha_t));

    struct json_object *root_obj = json_tokener_parse(conn.resp_buf);
    if (root_obj == NULL) {
        //error, need to get which one
        if (strncmp(conn.resp_buf, "Error:", 6) == 0) {
            err = get_err(conn.resp_buf + 7);
        }
        pps_printf("%s\n", "An error occured when parsing the string into a json object");
        ckvs_rpc_close(&conn);
        free(c2);
        ckvs_close(&ckvs);
        return err == ERR_NONE ? ERR_IO : err;
    }

    err = get_string(root_obj, "c2", c2_str);
    if (err != ERR_NONE) {
        //error, need to get which one
        char error[30];
        int err2 = get_string(root_obj, "error", error);
        if (err2 == ERR_NONE) {
            err = get_err(error);
        }
        ckvs_close(&ckvs);
        ckvs_rpc_close(&conn);
        free(c2);
        json_object_put(root_obj);
        return err;
    }



    err = SHA256_from_string(c2_str, c2);
    if (err != ERR_NONE) {
        //error
        ckvs_close(&ckvs);
        ckvs_rpc_close(&conn);
        free(c2);
        json_object_put(root_obj);
        return err;
    }

    unsigned char* data = calloc(conn.resp_size , sizeof(unsigned char));
    if (data == NULL) {
        //error
        ckvs_close(&ckvs);
        ckvs_rpc_close(&conn);
        free(c2);
        json_object_put(root_obj);
        return ERR_OUT_OF_MEMORY;
    }

    err = get_string(root_obj,"data",data);
    if (err != ERR_NONE) {
        //error
        free(data);
        ckvs_rpc_close(&conn);
        free(c2);
        json_object_put(root_obj);
        return err;
    }


    err = ckvs_client_compute_masterkey(&ckvs_mem, c2);
    if (err != ERR_NONE) {
        //error
        free(data);
        free(c2);
        ckvs_rpc_close(&conn);
        ckvs_close(&ckvs);
        json_object_put(root_obj);
        return err;
    }

    data = realloc(data, strlen(data) + 1);

    //initialize the string where the encrypted secret will be stored
    unsigned char *encrypted = calloc(strlen(data) / 2, sizeof(unsigned char));
    if (encrypted == NULL) {
        //error
        free(data);
        free(c2);
        ckvs_rpc_close(&conn);
        ckvs_close(&ckvs);
        json_object_put(root_obj);
        return ERR_OUT_OF_MEMORY;
    }


    err = hex_decode(data,encrypted);
    if (err == -1) {
        //error
        return ERR_IO;
    }

    //initialize the string where the decrypted secret will be stored
    size_t decrypted_len = strlen(data) / 2 + EVP_MAX_BLOCK_LENGTH;
    unsigned char *decrypted = calloc(decrypted_len, sizeof(unsigned char));

    if (decrypted == NULL) {
        //error
        free(data);
        free(c2);
        free(encrypted);
        ckvs_close(&ckvs);
        ckvs_rpc_close(&conn);
        json_object_put(root_obj);
        return ERR_OUT_OF_MEMORY;
    }

    //decrypts the string with the secret with in particular the master_key stored in ckvs_mem
    err = ckvs_client_crypt_value(&ckvs_mem, DECRYPTION, encrypted, strlen(data)/2, decrypted,
                                  &decrypted_len);
    if (err != ERR_NONE) {
        //error
        free(encrypted);
        free(data);
        free(c2);
        curl_free(ready_key);
        ckvs_close(&ckvs);
        free_uc(&decrypted);
        json_object_put(root_obj);
        ckvs_rpc_close(&conn);
        return err;
    }

    //check if we have to end the lecture
    for (size_t i = 0; i < decrypted_len; ++i) {
        if ((iscntrl(decrypted[i]) && decrypted[i] != '\n')) break;
        pps_printf("%c", decrypted[i]);
    }

    //free all objects
    free(encrypted);
    free(c2);
    free(data);
    json_object_put(root_obj);
    free_uc(&decrypted);
    ckvs_rpc_close(&conn);
    ckvs_close(&ckvs);

    return ERR_NONE;
}

/* *************************************************** *
 * TODO WEEK 13                                        *
 * *************************************************** */
int ckvs_client_set(const char *url, int optargc, char **optargv) {
    return NOT_IMPLEMENTED;
}

int ckvs_client_new(const char *url, int optargc, char **optargv) {
    return NOT_IMPLEMENTED;
}
