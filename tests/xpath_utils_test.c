/**
 * @file xpath_utils_test.c
 * @author Rastislav Szabo <raszabo@cisco.com>, Lukas Macko <lmacko@cisco.com>
 * @brief Sysrepo xpath_utils unit tests.
 *
 * @copyright
 * Copyright 2015 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <setjmp.h>
#include <cmocka.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#include "sysrepo/xpath.h"
#include "system_helper.h"

static void
sr_xpath_next_node_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_next_node(xpath, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_next_node_with_ns_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_next_node_with_ns(xpath, &state);
    assert_non_null(res);
    assert_string_equal(res, "example-module:container");

    res = sr_xpath_next_node_with_ns(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_next_node_with_ns(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_next_key_name_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_next_key_name(xpath, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_next_key_name(NULL, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_next_key_name(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "key1");

    res = sr_xpath_next_key_name(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "key2");

    res = sr_xpath_next_key_name(NULL, &state);
    assert_null(res);

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_next_key_value_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_next_key_value(xpath, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_next_key_value(NULL, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_next_key_value(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyA");

    res = sr_xpath_next_key_value(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_next_key_name(NULL, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_node(xpath, "leaf", &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    res = sr_xpath_node(NULL, "container",&state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node(NULL, "list",&state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    res = sr_xpath_node(NULL, "container",&state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node(NULL, "unknown", &state);
    assert_null(res);

    /* unsuccessful call left state untouched */
    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");


    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_rel_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_node_rel(xpath, "container", &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node_rel(NULL, "leaf", &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    res = sr_xpath_node(NULL, "list",&state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_idx_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_node_idx(xpath, 0, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node_idx(NULL, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_node_idx(NULL, 2,&state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    res = sr_xpath_node_idx(NULL, 100, &state);
    assert_null(res);

    res = sr_xpath_node_idx(NULL, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_idx_rel_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_node_idx_rel(xpath, 0, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node_idx_rel(NULL, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    res = sr_xpath_node_idx_rel(NULL, 0, &state);
    assert_null(res);

    sr_xpath_recover(&state);

    res = sr_xpath_node_idx_rel(xpath, 100, &state);
    assert_null(res);

    res = sr_xpath_node_idx_rel(NULL, 0, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_key_value_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_node_key_value(xpath, "abc", &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node_key_value(NULL, "unknown", &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_node_key_value(NULL, "key2", &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_node_key_value(NULL, "key1", &state);
    assert_non_null(res);
    assert_string_equal(res, "keyA");

    res = sr_xpath_node_key_value(NULL, "key2", &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_key_value_idx_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_node_key_value_idx(xpath, 0, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "container");

    res = sr_xpath_node_key_value_idx(NULL, 1, &state);
    assert_null(res);

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "list");

    res = sr_xpath_node_key_value_idx(NULL, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_node_key_value_idx(NULL, 0, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyA");

    res = sr_xpath_node_key_value_idx(NULL, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_next_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "leaf");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_key_value_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_key_value(xpath, "abc", "xyz", &state);
    assert_null(res);

    res = sr_xpath_key_value(NULL, "container", "xyz", &state);
    assert_null(res);

    res = sr_xpath_key_value(NULL, "list", "key1", &state);
    assert_non_null(res);
    assert_string_equal(res, "keyA");

    res = sr_xpath_key_value(NULL, "list", "key2", &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_key_value(NULL, "list", "key3", &state);
    assert_null(res);

    res = sr_xpath_key_value(NULL, "list", "key2", &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_key_value(NULL, "leaf", "abc", &state);
    assert_null(res);

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_with_augments_test (void **st)
{
    char xpath[] = "/ietf-interfaces:interfaces/interface[name='eth0']/ietf-ip:ipv4/address[ip='192.168.2.100']/prefix-length";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_next_node(xpath, &state);
    assert_non_null(res);
    assert_string_equal(res, "interfaces");

    res = sr_xpath_key_value(NULL, "address", "ip", &state);
    assert_non_null(res);
    assert_string_equal(res, "192.168.2.100");

    res = sr_xpath_node(NULL, "interface", &state);
    assert_non_null(res);
    assert_string_equal(res, "interface");

    res = sr_xpath_next_node_with_ns(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "ietf-ip:ipv4");

    res = sr_xpath_last_node(NULL, &state);
    assert_non_null(res);
    assert_string_equal(res, "prefix-length");

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/ietf-interfaces:interfaces/interface[name='eth0']/ietf-ip:ipv4/address[ip='192.168.2.100']/prefix-length");

}

static void
sr_xpath_key_value_idx_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_key_value_idx(xpath, 10, 5, &state);
    assert_null(res);

    res = sr_xpath_key_value_idx(NULL, 0, 0, &state);
    assert_null(res);

    res = sr_xpath_key_value_idx(NULL, 1, 0, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyA");

    res = sr_xpath_key_value_idx(NULL, 1, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_key_value_idx(NULL, 1, 2, &state);
    assert_null(res);

    res = sr_xpath_key_value_idx(NULL, 1, 1, &state);
    assert_non_null(res);
    assert_string_equal(res, "keyB");

    res = sr_xpath_key_value_idx(NULL, 2, 2, &state);
    assert_null(res);

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_last_node_test (void **st)
{
    char xpath[] = "/example-module:container/list[key1='keyA'][key2='keyB']/leaf";
    sr_xpath_ctx_t state = {0};

    char *res = NULL;

    res = sr_xpath_last_node(xpath, &state);
    assert_non_null(res);
    assert_string_equal("leaf", res);

    res = sr_xpath_last_node(xpath, &state);
    assert_non_null(res);
    assert_string_equal("leaf", res);

    sr_xpath_recover(&state);

    assert_string_equal(xpath, "/example-module:container/list[key1='keyA'][key2='keyB']/leaf");

}

static void
sr_xpath_node_name_test (void **st)
{
    char *res = NULL;

    res = sr_xpath_node_name("/example-module:container/list[key1='keyA'][key2='keyB']/leaf");
    assert_non_null(res);
    assert_string_equal("leaf", res);

    res = sr_xpath_node_name("/example-module:container/list[key1='keyA'][key2='keyB']");
    assert_non_null(res);
    assert_string_equal("list[key1='keyA'][key2='keyB']", res);
}

static void
sr_xpath_node_name_eq_test (void **st)
{
    bool res = false;

    res = sr_xpath_node_name_eq("/example-module:container/list[key1='keyA'][key2='keyB']/leaf", "leaf");
    assert_true(res);

    res = sr_xpath_node_name_eq("/example-module:container/list[key1='keyA'][key2='keyB']/leaf", "/leaf");
    assert_false(res);

    res = sr_xpath_node_name_eq("/example-module:container/list[key1='keyA'][key2='keyB']", "list[key1='keyA'][key2='keyB']");
    assert_true(res);

    res = sr_xpath_node_name_eq("/example-module:container/list[key1='keyA'][key2='keyB']", "list");
    assert_false(res);
}

int
main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(sr_xpath_next_node_test),
        cmocka_unit_test(sr_xpath_next_node_with_ns_test),
        cmocka_unit_test(sr_xpath_next_key_name_test),
        cmocka_unit_test(sr_xpath_next_key_value_test),
        cmocka_unit_test(sr_xpath_node_test),
        cmocka_unit_test(sr_xpath_node_rel_test),
        cmocka_unit_test(sr_xpath_node_idx_test),
        cmocka_unit_test(sr_xpath_node_idx_rel_test),
        cmocka_unit_test(sr_xpath_node_key_value_test),
        cmocka_unit_test(sr_xpath_node_key_value_idx_test),
        cmocka_unit_test(sr_xpath_key_value_test),
        cmocka_unit_test(sr_xpath_key_value_idx_test),
        cmocka_unit_test(sr_xpath_last_node_test),
        cmocka_unit_test(sr_xpath_node_name_test),
        cmocka_unit_test(sr_xpath_node_name_eq_test),
        cmocka_unit_test(sr_xpath_with_augments_test),
    };

    watchdog_start(300);
    int ret = cmocka_run_group_tests(tests, NULL, NULL);
    watchdog_stop();
    return ret;
}
