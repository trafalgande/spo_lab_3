#include <malloc.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "events_module.h"
#include "ui_module.h"


list_item_t* find_transfer_progress(list_item_t* transfer_list, transfer_progress_t *tp) {
    list_item_t *item = transfer_list;
    while (item != NULL && tp != NULL) {
        transfer_progress_t *item_data = item->data;
        if (item_data &&
            0 == strcmp(item_data->triplet.filename, tp->triplet.filename) &&
            0 == strncmp(item_data->triplet.hash, tp->triplet.hash, 32)
            ) {
            return item;
        }
        item = item->next;
    }
    return NULL;
}

void destroy_transfer_progress(transfer_progress_t *tp) {
    free(tp);
}

void destroy_action_message(char *str) {
    free(str);
}

list_item_t* find_download(events_module_data_t* em, transfer_progress_t *tp) {
    pthread_mutex_lock(&em->download_mutex);
    list_item_t *item = find_transfer_progress(em->download_list, tp);
    pthread_mutex_unlock(&em->download_mutex);
    return item;
}

list_item_t* find_upload(events_module_data_t* em, transfer_progress_t *tp) {
    pthread_mutex_lock(&em->upload_mutex);
    list_item_t *item = find_transfer_progress(em->upload_list, tp);
    pthread_mutex_unlock(&em->upload_mutex);
    return item;
}

void init_events_module(events_module_data_t* em) {
    pthread_mutex_init(&em->actions_mutex, NULL);
    pthread_mutex_init(&em->download_mutex, NULL);
    pthread_mutex_init(&em->upload_mutex, NULL);
    em->actions_list = calloc(1, sizeof(list_item_t));
    em->download_list = calloc(1, sizeof(list_item_t));
    em->upload_list = calloc(1, sizeof(list_item_t));
}

void destroy_events_module(events_module_data_t* em) {
    pthread_mutex_destroy(&em->actions_mutex);
    pthread_mutex_destroy(&em->download_mutex);
    pthread_mutex_destroy(&em->upload_mutex);
    destroy_list(em->download_list, (int (*)(void *)) destroy_transfer_progress);
    destroy_list(em->upload_list, (int (*)(void *)) destroy_transfer_progress);
    destroy_list(em->actions_list, (int (*)(void *)) destroy_action_message);
}

void put_download(events_module_data_t* em, transfer_progress_t *progress) {
    pthread_mutex_lock(&em->download_mutex);
    list_item_t *found = find_transfer_progress(em->download_list, progress);
    int8_t do_clear = 0;
    if (found) {
        transfer_progress_t *data = found->data;
        data->transferred = progress->transferred;
    } else {
        em->download_list = push(em->download_list, progress);
        do_clear = 1;
    }
        render_transfer_area(em->ui_data, do_clear);
    pthread_mutex_unlock(&em->download_mutex);
}

void del_download(events_module_data_t* em, transfer_progress_t *progress) {
    pthread_mutex_lock(&em->download_mutex);
    list_item_t *found = find_transfer_progress(em->download_list, progress);
    if (found) {
        em->download_list = remove_el(em->download_list, found);
        if (!em->download_list) {
            put_action(em, "[EVENTS-MODULE] ERROR on del download");
        }
    } else {
        // Apparently, deleted by other thread
    }
    render_transfer_area(em->ui_data, 1);
    pthread_mutex_unlock(&em->download_mutex);
}
void put_upload(events_module_data_t* em, transfer_progress_t *progress) {
    pthread_mutex_lock(&em->upload_mutex);
    list_item_t *found = find_transfer_progress(em->upload_list, progress);
    int8_t do_clear = 0;
    if (found) {
        transfer_progress_t *data = found->data;
        data->transferred = progress->transferred;
    } else {
        em->upload_list = push(em->upload_list, progress);
        do_clear = 1;
    }
    render_transfer_area(em->ui_data, do_clear);
    pthread_mutex_unlock(&em->upload_mutex);
}
void del_upload(events_module_data_t* em, transfer_progress_t *progress) {
    pthread_mutex_lock(&em->upload_mutex);
    list_item_t *found = find_transfer_progress(em->upload_list, progress);
    if (found) {
        em->upload_list = remove_el(em->upload_list, found);
        if (!em->upload_list) {
            put_action(em, "[EVENTS-MODULE] ERROR on del upload");
        }
    } else {
        // Apparently, deleted by other thread
    }
    render_transfer_area(em->ui_data, 1);
    pthread_mutex_unlock(&em->upload_mutex);
}

void add_time_tag(char *str) {
    time_t timer;
    struct tm* tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(str, 29, "[%Y-%m-%d %H:%M:%S] ", tm_info);
}

void put_action(events_module_data_t* em, const char *str) {
    pthread_mutex_lock(&em->actions_mutex);
    char *logged_str = calloc(1, 512);
    add_time_tag(logged_str);
    strcat(logged_str, str);
    em->actions_list = push(em->actions_list, logged_str);
    render_events_log(em->ui_data, 1);
    pthread_mutex_unlock(&em->actions_mutex);
}

void log_error(events_module_data_t *em, char *msg) {
    char error[256] = {0};
    sprintf(error, msg, errno);
    put_action(em, error);
}

void log_action(events_module_data_t* em, const char *msg, const char *arg) {
    char action[256] = {0};
    sprintf(action, msg, arg);
    put_action(em, action);
}