#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <types.h>

void load_config(const uint8_t* persisted_config);
PersistConfigReturnCode persist_config();

uint16_t handle_get_report1(uint8_t report_id, uint8_t* buffer, uint16_t reqlen);
void handle_set_report1(uint8_t report_id, uint8_t const* buffer, uint16_t bufsize);

void reset_resolution_multiplier();

bool is_config_authorized();
void reset_auth_session();
void load_auth_config(const uint8_t* data, size_t len);
void do_persist_auth(const uint8_t* buffer, size_t len);

void get_bonded_device_info(uint32_t index, bonded_device_response_t* out);
void unpair_single_device(uint8_t addr_type, const uint8_t addr[6]);

#endif


