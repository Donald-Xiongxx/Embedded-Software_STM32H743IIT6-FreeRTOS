#include "flash_storage.h"
#include "stm32h7xx_hal.h"
#include <string.h>

#define FLASH_BASE_ADDR     0x08000000
#define FLASH_END_ADDR     0x081FFFFF
#define DB_SECTOR_SIZE     0x20000

static uint32_t GetSector(uint32_t Address)
{
    uint32_t sector = 0;
    if((Address < 0x08100000) && (Address >= 0x08040000))
    {
        sector = (Address - 0x08040000) / 0x20000 + 10;
    }
    else if(Address >= 0x08100000)
    {
        sector = (Address - 0x08100000) / 0x20000 + 18;
    }
    return sector;
}

int flash_storage_init(void)
{
    return 0;
}

uint32_t flash_storage_get_start(void)
{
    return FLASH_END_ADDR - DB_SECTOR_SIZE + 1;
}

uint32_t flash_storage_get_size(void)
{
    return DB_SECTOR_SIZE;
}

int flash_storage_read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    uint32_t addr = flash_storage_get_start() + offset;
    if(offset + len > DB_SECTOR_SIZE)
        return -1;
    memcpy(buf, (void*)addr, len);
    return 0;
}

int flash_storage_write(uint32_t offset, const uint8_t *buf, uint32_t len)
{
    uint32_t addr = flash_storage_get_start() + offset;
    uint32_t sector;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;

    if(offset + len > DB_SECTOR_SIZE)
        return -1;

    HAL_FLASH_Unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Banks = FLASH_BANK_2;
    EraseInitStruct.Sector = GetSector(addr);
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return -1;
    }

    for(uint32_t i = 0; i < len; i += 32)
    {
        uint32_t data[8] = {0};
        uint32_t copy_len = (len - i) < 32 ? (len - i) : 32;
        memcpy(data, buf + i, copy_len);
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr + i, (uint32_t)data) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}
