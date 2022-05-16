/**
 * @file ckvs_client.h
 * @brief client-side operations over network
 * @author E Bugnion, A. Clergeot
 */
#pragma once

/* *************************************************** *
 * TODO WEEK 10                                        *
 * *************************************************** */
/**
 * @brief Performs the 'stats' command by connecting to the remote server at url.
 *
 * @param url (const char*) the url of the remote CKVS server
 * @param optargc (int) the number of optional arguments that are provided (should be 0)
 * @param optargv (char**) the values of optional arguments that were provided
 * @return int, error code
 */
int ckvs_client_stats(const char *url, int optargc, char **optargv);

/**
 * @brief Performs the 'get' command by connecting to the remote server at url.
 *
 * @param url (const char*) the url of the remote CKVS server
 * @param optargc (int) the number of optional arguments that are provided (should be 2)
 * @param optargv (char**) the values of optional arguments that were provided
 * @return int, error code
 */
int ckvs_client_get(const char *url, int optargc, char **optargv);

/* *************************************************** *
 * TODO WEEK 13                                        *
 * *************************************************** */
/**
 * @brief Performs the 'set' command by connecting to the remote server at url.
 *
 * @param url (const char*) the url of the remote CKVS server
 * @param optargc (int) the number of optional arguments that are provided (should be 3)
 * @param optargv (char**) the values of optional arguments that were provided
 * @return int, error code
 */
int ckvs_client_set(const char *url, int optargc, char **optargv);

/**
 * @brief Performs the 'new' command by connecting to the remote server at url.
 *
 * @param url (const char*) the url of the remote CKVS server
 * @param optargc (int) the number of optional arguments that are provided (should be 2)
 * @param optargv (char**) the values of optional arguments that were provided
 * @return int, error code
 */
int ckvs_client_new(const char *url, int optargc, char **optargv);



/**
 * @brief Retrives the ckvs from a json object.
 *
 * @param ckvs (struct ckvs*) the ckvs in which we put the read content
 * @param obj (const json_object*) the json object we retrieve the information from
 * @return int, error code
 */
int retrieve_ckvs_from_json(struct CKVS* ckvs, const struct json_object* obj);


/**
 * @brief Retrives the ckvs header from a json object.
 *
 * @param ckvs (struct ckvs*) the ckvs in which we put the read content for the header
 * @param obj (const json_object*) the json object we retrieve the information from
 * @return int, error code
 */
int retrieve_ckvs_header_from_json(struct CKVS *ckvs, const struct json_object *obj);

/**
 * @brief To get the usable string associated to the key from a json object.
 *
 * @param obj (const json_object*) the json object we retrieve the information from
 * @param key (const char*) the key of the string of interest
 * @param buf (char*) buffer used to store the computed string
 * @return int, error code
 */
int get_string(const struct json_object* obj, const char* key, char* buf);

/**
 * @brief To get the integer associated to the key from a json object.
 *
 * @param obj (const json_object*) the json object we retrieve the information from
 * @param key (const char*) the key of the integer of interest
 * @param buf (char*) buffer used to store the computed integer
 * @return int, error code
 */
int get_int(const struct json_object* obj, const char* key, int* buf);



