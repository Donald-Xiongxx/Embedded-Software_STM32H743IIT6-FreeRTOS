#include "sqlite_task.h"
#include "sqlite3.h"
#include "flash_storage.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

static sqlite3 *db = NULL;

typedef struct {
    const struct sqlite3_io_methods *pMethods;
    uint32_t flash_offset;
} flash_file;

static int flash_close(sqlite3_file *pFile)
{
    (void)pFile;
    return SQLITE_OK;
}

static int flash_read(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite3_int64 iOfst)
{
    flash_file *p = (flash_file*)pFile;
    uint32_t offset = p->flash_offset + (uint32_t)iOfst;
    if(offset + iAmt > flash_storage_get_size())
        return SQLITE_IOERR_SHORT_READ;
    if(flash_storage_read(offset, (uint8_t*)zBuf, iAmt) != 0)
        return SQLITE_IOERR_READ;
    return SQLITE_OK;
}

static int flash_write(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite3_int64 iOfst)
{
    flash_file *p = (flash_file*)pFile;
    uint32_t offset = p->flash_offset + (uint32_t)iOfst;
    if(offset + iAmt > flash_storage_get_size())
        return SQLITE_IOERR_WRITE;
    if(flash_storage_write(offset, (const uint8_t*)zBuf, iAmt) != 0)
        return SQLITE_IOERR_WRITE;
    return SQLITE_OK;
}

static int flash_truncate(sqlite3_file *pFile, sqlite3_int64 size)
{
    (void)pFile;
    (void)size;
    return SQLITE_OK;
}

static int flash_sync(sqlite3_file *pFile, int flags)
{
    (void)pFile;
    (void)flags;
    return SQLITE_OK;
}

static int flash_filesize(sqlite3_file *pFile, sqlite3_int64 *pSize)
{
    (void)pFile;
    *pSize = flash_storage_get_size();
    return SQLITE_OK;
}

static int flash_lock(sqlite3_file *pFile, int eLock)
{
    (void)pFile;
    (void)eLock;
    return SQLITE_OK;
}

static int flash_unlock(sqlite3_file *pFile, int eLock)
{
    (void)pFile;
    (void)eLock;
    return SQLITE_OK;
}

static int flash_checkreserved_lock(sqlite3_file *pFile, int *pResOut)
{
    (void)pFile;
    *pResOut = 0;
    return SQLITE_OK;
}

static int flash_file_control(sqlite3_file *pFile, int op, void *pArg)
{
    (void)pFile;
    (void)op;
    (void)pArg;
    return SQLITE_NOTFOUND;
}

static int flash_sector_size(sqlite3_file *pFile)
{
    (void)pFile;
    return 4096;
}

static int flash_device_characteristics(sqlite3_file *pFile)
{
    (void)pFile;
    return 0;
}

static int flash_shm_map(sqlite3_file *pFile, int iPg, int pgsz, int bRead, void volatile **pp)
{
    (void)pFile;
    (void)iPg;
    (void)pgsz;
    (void)bRead;
    (void)pp;
    return SQLITE_OK;
}

static int flash_shm_lock(sqlite3_file *pFile, int offset, int n, int flags)
{
    (void)pFile;
    (void)offset;
    (void)n;
    (void)flags;
    return SQLITE_OK;
}

static void flash_shm_barrier(sqlite3_file *pFile)
{
    (void)pFile;
}

static int flash_shm_unmap(sqlite3_file *pFile, int deleteFlag)
{
    (void)pFile;
    (void)deleteFlag;
    return SQLITE_OK;
}

static int flash_fetch(sqlite3_file *pFile, sqlite3_int64 iOfst, int iAmt, void **pp)
{
    (void)pFile;
    (void)iOfst;
    (void)iAmt;
    (void)pp;
    return SQLITE_OK;
}

static int flash_unfetch(sqlite3_file *pFile, sqlite3_int64 iOfst, void *p)
{
    (void)pFile;
    (void)iOfst;
    (void)p;
    return SQLITE_OK;
}

static const struct sqlite3_io_methods flash_io_methods = {
    .iVersion = 3,
    .xClose = flash_close,
    .xRead = flash_read,
    .xWrite = flash_write,
    .xTruncate = flash_truncate,
    .xSync = flash_sync,
    .xFileSize = flash_filesize,
    .xLock = flash_lock,
    .xUnlock = flash_unlock,
    .xCheckReservedLock = flash_checkreserved_lock,
    .xFileControl = flash_file_control,
    .xSectorSize = flash_sector_size,
    .xDeviceCharacteristics = flash_device_characteristics,
    .xShmMap = flash_shm_map,
    .xShmLock = flash_shm_lock,
    .xShmBarrier = flash_shm_barrier,
    .xShmUnmap = flash_shm_unmap,
    .xFetch = flash_fetch,
    .xUnfetch = flash_unfetch,
};

static int flash_vfs_open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile, int flags, int *pOutFlags)
{
    (void)pVfs;
    (void)zName;
    (void)pOutFlags;

    flash_file *p = (flash_file*)pFile;
    p->pMethods = &flash_io_methods;
    p->flash_offset = 0;

    (void)flags;
    return SQLITE_OK;
}

static int flash_vfs_delete(sqlite3_vfs *pVfs, const char *zName, int syncDir)
{
    (void)pVfs;
    (void)zName;
    (void)syncDir;
    return SQLITE_OK;
}

static int flash_vfs_access(sqlite3_vfs *pVfs, const char *zName, int flags, int *pResOut)
{
    (void)pVfs;

    if(flags == SQLITE_ACCESS_EXISTS)
    {
        *pResOut = 1;
        return SQLITE_OK;
    }
    *pResOut = 0;
    return SQLITE_OK;
}

static int flash_vfs_fullpathname(sqlite3_vfs *pVfs, const char *zName, int nOut, char *zOut)
{
    (void)pVfs;
    strncpy(zOut, zName, nOut - 1);
    zOut[nOut - 1] = 0;
    return SQLITE_OK;
}

static void *flash_vfs_dlopen(sqlite3_vfs *pVfs, const char *zFilename)
{
    (void)pVfs;
    (void)zFilename;
    return NULL;
}

static void flash_vfs_dlerror(sqlite3_vfs *pVfs, int nByte, char *zErrMsg)
{
    (void)pVfs;
    if(nByte > 0 && zErrMsg != NULL)
        zErrMsg[0] = '\0';
}

static void (*flash_vfs_dlsym(sqlite3_vfs *pVfs, void *p, const char *zSymbol))(void)
{
    (void)pVfs;
    (void)p;
    (void)zSymbol;
    return NULL;
}

static void flash_vfs_dlclose(sqlite3_vfs *pVfs, void *pHandle)
{
    (void)pVfs;
    (void)pHandle;
}

static int flash_vfs_randomness(sqlite3_vfs *pVfs, int nByte, char *zOut)
{
    (void)pVfs;
    for(int i = 0; i < nByte; i++)
        zOut[i] = (char)((i * 17 + 37) & 0xFF);
    return nByte;
}

static int flash_vfs_sleep(sqlite3_vfs *pVfs, int microseconds)
{
    (void)pVfs;
    return microseconds;
}

static int flash_vfs_currenttime(sqlite3_vfs *pVfs, double *prNow)
{
    (void)pVfs;
    *prNow = 0.0;
    return SQLITE_OK;
}

static int flash_vfs_get_last_error(sqlite3_vfs *pVfs, int e, char *z)
{
    (void)pVfs;
    (void)e;
    (void)z;
    return 0;
}

static int flash_vfs_currenttime_int64(sqlite3_vfs *pVfs, sqlite3_int64 *piNow)
{
    (void)pVfs;
    *piNow = 0;
    return SQLITE_OK;
}

static int flash_vfs_set_system_call(sqlite3_vfs *pVfs, const char *zName, sqlite3_syscall_ptr p)
{
    (void)pVfs;
    (void)zName;
    (void)p;
    return SQLITE_NOTFOUND;
}

static sqlite3_syscall_ptr flash_vfs_get_system_call(sqlite3_vfs *pVfs, const char *zName)
{
    (void)pVfs;
    (void)zName;
    return NULL;
}

static const char *flash_vfs_next_system_call(sqlite3_vfs *pVfs, const char *zName)
{
    (void)pVfs;
    (void)zName;
    return NULL;
}

static sqlite3_vfs flash_vfs = {
    .iVersion = 3,
    .szOsFile = sizeof(flash_file),
    .mxPathname = 256,
    .pNext = NULL,
    .zName = "flash_vfs",
    .pAppData = NULL,
    .xOpen = flash_vfs_open,
    .xDelete = flash_vfs_delete,
    .xAccess = flash_vfs_access,
    .xFullPathname = flash_vfs_fullpathname,
    .xDlOpen = flash_vfs_dlopen,
    .xDlError = flash_vfs_dlerror,
    .xDlSym = flash_vfs_dlsym,
    .xDlClose = flash_vfs_dlclose,
    .xRandomness = flash_vfs_randomness,
    .xSleep = flash_vfs_sleep,
    .xCurrentTime = flash_vfs_currenttime,
    .xGetLastError = flash_vfs_get_last_error,
    .xCurrentTimeInt64 = flash_vfs_currenttime_int64,
    .xSetSystemCall = flash_vfs_set_system_call,
    .xGetSystemCall = flash_vfs_get_system_call,
    .xNextSystemCall = flash_vfs_next_system_call,
};

int sqlite3_os_init(void)
{
    return sqlite3_vfs_register(&flash_vfs, 1);
}

int sqlite3_os_end(void)
{
    return SQLITE_OK;
}

int sqlite_task_init(void)
{
    int result;

    result = sqlite3_open_v2("sensor.db", &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX, "flash_vfs");
    if(result != SQLITE_OK)
    {
        if(db != NULL)
        {
            sqlite3_close(db);
            db = NULL;
        }
        result = sqlite3_open_v2("sensor.db", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, "flash_vfs");
        if(result != SQLITE_OK)
        {
            return -1;
        }

        const char *sql = "CREATE TABLE IF NOT EXISTS sensor_data ("
                          "id INTEGER PRIMARY KEY, "
                          "timestamp INTEGER NOT NULL, "
                          "temperature REAL, "
                          "humidity REAL);";

        char *err_msg = NULL;
        if(sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK)
        {
            if(err_msg != NULL)
            {
                sqlite3_free(err_msg);
            }
            sqlite3_close(db);
            db = NULL;
            return -1;
        }
    }

    return 0;
}

int sqlite_task_insert(float temperature, float humidity)
{
    if(db == NULL)
        return -1;

    char sql[128];
    int64_t timestamp = HAL_GetTick();

    snprintf(sql, sizeof(sql),
             "INSERT INTO sensor_data (timestamp, temperature, humidity) VALUES (%lld, %.2f, %.2f);",
             (long long)timestamp, temperature, humidity);

    char *err_msg = NULL;
    if(sqlite3_exec(db, sql, NULL, NULL, &err_msg) != SQLITE_OK)
    {
        if(err_msg != NULL)
        {
            sqlite3_free(err_msg);
        }
        return -1;
    }

    return 0;
}

int sqlite_task_query_latest(float *temperature, float *humidity)
{
    if(db == NULL)
        return -1;

    const char *sql = "SELECT temperature, humidity FROM sensor_data ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if(result != SQLITE_OK)
        return -1;

    result = sqlite3_step(stmt);
    if(result == SQLITE_ROW)
    {
        *temperature = (float)sqlite3_column_double(stmt, 0);
        *humidity = (float)sqlite3_column_double(stmt, 1);
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return -1;
}

void vTaskSQLite(void *pvParameters)
{
    (void)pvParameters;

    flash_storage_init();

    if(sqlite_task_init() != 0)
    {
        for(;;)
        {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
            vTaskDelay(pdMS_TO_TICKS(100));
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    for(int i = 0; i < 5; i++)
    {
        sqlite_task_insert(25.0f + i * 0.5f, 60.0f + i * 2.0f);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    float temp, humi;
    if(sqlite_task_query_latest(&temp, &humi) == 0)
    {
    }

    vTaskDelay(portMAX_DELAY);
}
