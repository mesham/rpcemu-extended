#ifndef EMULATOR_SNAPSHOT_H
#define EMULATOR_SNAPSHOT_H

#include <cstdint>
#include <string>
#include <vector>

#include "machine_snapshot.h"

MachineSnapshot emulator_take_snapshot();
std::vector<uint8_t> emulator_read_memory(uint32_t address, uint32_t length);
std::string emulator_disassemble_at(uint32_t address, int count);

#endif
