#include "sqlite_task.h"
#include "sqlite3.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

static sqlite3 *db = NULL;

#define RAM_DB_SIZE (128 * 1024)  // 128KB RAM for SQLite database (增加空间避免溢出)

static uint8_t g_ram_db_storage[RAM_DB_SIZE] __attribute__((section(".ram_storage")));

typedef struct {
    const struct sqlite3_io_methods *pMethods;
    uint32_t file_size;
    uint8_t *data;
} ram_file;

static int ram_close(sqlite3_file *pFile)
{
    printf("[VFS] ram_close called\r\n");
    (void)pFile;
    return SQLITE_OK;
}

static int ram_read(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite3_int64 iOfst)
{
    ram_file *p = (ram_file*)pFile;
    
    if(iOfst < 0 || iAmt < 0)
        return SQLITE_IOERR_READ;
    
    if((uint32_t)iOfst + (uint32_t)iAmt > p->file_size)
    {
        if((uint32_t)iOfst >= p->file_size)
        {
            memset(zBuf, 0, iAmt);
            return SQLITE_IOERR_SHORT_READ;
        }
        memset(((uint8_t*)zBuf) + (p->file_size - (uint32_t)iOfst), 0, 
               iAmt - (p->file_size - (uint32_t)iOfst));
    }
    
    memcpy(zBuf, p->data + (uint32_t)iOfst, iAmt);
    return SQLITE_OK;
}

static int ram_write(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite3_int64 iOfst)
{
    ram_file *p = (ram_file*)pFile;
    
    if(iOfst < 0 || iAmt < 0)
        return SQLITE_IOERR_WRITE;
    
    if((uint32_t)iOfst + (uint32_t)iAmt > RAM_DB_SIZE)
        return SQLITE_IOERR_WRITE;
    
    if((uint32_t)iOfst + (uint32_t)iAmt > p->file_size)
        p->file_size = (uint32_t)iOfst + (uint32_t)iAmt;
    
    memcpy(p->data + (uint32_t)iOfst, zBuf, iAmt);
    return SQLITE_OK;
}

static int ram_truncate(sqlite3_file *pFile, sqlite3_int64 size)
{
    ram_file *p = (ram_file*)pFile;
    if((uint32_t)size < p->file_size)
        p->file_size = (uint32_t)size;
    return SQLITE_OK;
}

static int ram_sync(sqlite3_file *pFile, int flags)
{
    (void)pFile;
    (void)flags;
    return SQLITE_OK;
}

static int ram_filesize(sqlite3_file *pFile, sqlite3_int64 *pSize)
{
    ram_file *p = (ram_file*)pFile;
    *pSize = p->file_size;
    return SQLITE_OK;
}

static int ram_lock(sqlite3_file *pFile, int eLock)
{
    (void)pFile;
    (void)eLock;
    return SQLITE_OK;
}

static int ram_unlock(sqlite3_file *pFile, int eLock)
{
    (void)pFile;
    (void)eLock;
    return SQLITE_OK;
}

static int ram_checkreserved_lock(sqlite3_file *pFile, int *pResOut)
{
    (void)pFile;
    *pResOut = 0;
    return SQLITE_OK;
}

static int ram_file_control(sqlite3_file *pFile, int op, void *pArg)
{
    (void)pFile;
    (void)op;
    (void)pArg;
    return SQLITE_NOTFOUND;
}

static int ram_sector_size(sqlite3_file *pFile)
{
    (void)pFile;
    return 512;
}

static int ram_device_characteristics(sqlite3_file *pFile)
{
    (void)pFile;
    return 0;
}

static int ram_shm_map(sqlite3_file *pFile, int iPg, int pgsz, int bRead, void volatile **pp)
{
    (void)pFile;
    (void)iPg;
    (void)pgsz;
    (void)bRead;
    (void)pp;
    return SQLITE_OK;
}

static int ram_shm_lock(sqlite3_file *pFile, int offset, int n, int flags)
{
    (void)pFile;
    (void)offset;
    (void)n;
    (void)flags;
    return SQLITE_OK;
}

static void ram_shm_barrier(sqlite3_file *pFile)
{
    (void)pFile;
}

static int ram_shm_unmap(sqlite3_file *pFile, int deleteFlag)
{
    (void)pFile;
    (void)deleteFlag;
    return SQLITE_OK;
}

static int ram_fetch(sqlite3_file *pFile, sqlite3_int64 iOfst, int iAmt, void **pp)
{
    (void)pFile;
    (void)iOfst;
    (void)iAmt;
    (void)pp;
    return SQLITE_OK;
}

static int ram_unfetch(sqlite3_file *pFile, sqlite3_int64 iOfst, void *p)
{
    (void)pFile;
    (void)iOfst;
    (void)p;
    return SQLITE_OK;
}

static const struct sqlite3_io_methods ram_io_methods = {
    .iVersion = 3,
    .xClose = ram_close,
    .xRead = ram_read,
    .xWrite = ram_write,
    .xTruncate = ram_truncate,
    .xSync = ram_sync,
    .xFileSize = ram_filesize,
    .xLock = ram_lock,
    .xUnlock = ram_unlock,
    .xCheckReservedLock = ram_checkreserved_lock,
    .xFileControl = ram_file_control,
    .xSectorSize = ram_sector_size,
    .xDeviceCharacteristics = ram_device_characteristics,
    .xShmMap = ram_shm_map,
    .xShmLock = ram_shm_lock,
    .xShmBarrier = ram_shm_barrier,
    .xShmUnmap = ram_shm_unmap,
    .xFetch = ram_fetch,
    .xUnfetch = ram_unfetch,
};

static int ram_vfs_open(sqlite3_vfs *pVfs, const char *zName, sqlite3_file *pFile, int flags, int *pOutFlags)
{
    printf("[VFS] ram_vfs_open: name=%s, flags=0x%x\r\n", zName ? zName : "NULL", flags);
    
    (void)pVfs;
    (void)pOutFlags;
    
    ram_file *p = (ram_file*)pFile;
    p->pMethods = NULL;
    p->file_size = 0;
    p->data = g_ram_db_storage;
    
    if(flags & SQLITE_OPEN_CREATE)
    {
        printf("[VFS] ram_vfs_open: Clearing %dKB RAM storage\r\n", RAM_DB_SIZE / 1024);
        memset(g_ram_db_storage, 0, RAM_DB_SIZE);
    }
    
    p->pMethods = &ram_io_methods;
    printf("[VFS] ram_vfs_open: SUCCESS\r\n");
    return SQLITE_OK;
}

static int ram_vfs_delete(sqlite3_vfs *pVfs, const char *zName, int syncDir)
{
    (void)pVfs;
    (void)zName;
    (void)syncDir;
    printf("[VFS] ram_vfs_delete: name=%s\r\n", zName ? zName : "NULL");
    return SQLITE_OK;
}

static int ram_vfs_access(sqlite3_vfs *pVfs, const char *zName, int flags, int *pResOut)
{
    (void)pVfs;
    
    printf("[VFS] ram_vfs_access: name=%s, flags=%d\r\n", zName ? zName : "NULL", flags);
    
    if(flags == SQLITE_ACCESS_EXISTS)
    {
        *pResOut = 0;
        return SQLITE_OK;
    }
    else if(flags == SQLITE_ACCESS_READWRITE)
    {
        *pResOut = 1;
        return SQLITE_OK;
    }
    else if(flags == SQLITE_ACCESS_READ)
    {
        *pResOut = 1;
        return SQLITE_OK;
    }
    *pResOut = 0;
    return SQLITE_OK;
}

static int ram_vfs_fullpathname(sqlite3_vfs *pVfs, const char *zName, int nOut, char *zOut)
{
    (void)pVfs;
    strncpy(zOut, zName, nOut - 1);
    zOut[nOut - 1] = 0;
    return SQLITE_OK;
}

static void *ram_vfs_dlopen(sqlite3_vfs *pVfs, const char *zFilename)
{
    (void)pVfs;
    (void)zFilename;
    return NULL;
}

static void ram_vfs_dlerror(sqlite3_vfs *pVfs, int nByte, char *zErrMsg)
{
    (void)pVfs;
    if(nByte > 0 && zErrMsg != NULL)
        zErrMsg[0] = '\0';
}

static void (*ram_vfs_dlsym(sqlite3_vfs *pVfs, void *p, const char *zSymbol))(void)
{
    (void)pVfs;
    (void)p;
    (void)zSymbol;
    return NULL;
}

static void ram_vfs_dlclose(sqlite3_vfs *pVfs, void *pHandle)
{
    (void)pVfs;
    (void)pHandle;
}

static int ram_vfs_randomness(sqlite3_vfs *pVfs, int nByte, char *zOut)
{
    (void)pVfs;
    for(int i = 0; i < nByte; i++)
        zOut[i] = (char)((i * 17 + 37) & 0xFF);
    return nByte;
}

static int ram_vfs_sleep(sqlite3_vfs *pVfs, int microseconds)
{
    (void)pVfs;
    return microseconds;
}

static int ram_vfs_currenttime(sqlite3_vfs *pVfs, double *prNow)
{
    (void)pVfs;
    *prNow = 0.0;
    return SQLITE_OK;
}

static int ram_vfs_get_last_error(sqlite3_vfs *pVfs, int e, char *z)
{
    (void)pVfs;
    (void)e;
    (void)z;
    return 0;
}

static int ram_vfs_currenttime_int64(sqlite3_vfs *pVfs, sqlite3_int64 *piNow)
{
    (void)pVfs;
    *piNow = 0;
    return SQLITE_OK;
}

static int ram_vfs_set_system_call(sqlite3_vfs *pVfs, const char *zName, sqlite3_syscall_ptr p)
{
    (void)pVfs;
    (void)zName;
    (void)p;
    return SQLITE_NOTFOUND;
}

static sqlite3_syscall_ptr ram_vfs_get_system_call(sqlite3_vfs *pVfs, const char *zName)
{
    (void)pVfs;
    (void)zName;
    return NULL;
}

static const char *ram_vfs_next_system_call(sqlite3_vfs *pVfs, const char *zName)
{
    (void)pVfs;
    (void)zName;
    return NULL;
}

static sqlite3_vfs ram_vfs = {
    .iVersion = 3,
    .szOsFile = sizeof(ram_file),
    .mxPathname = 256,
    .pNext = NULL,
    .zName = "ram_vfs",
    .pAppData = NULL,
    .xOpen = ram_vfs_open,
    .xDelete = ram_vfs_delete,
    .xAccess = ram_vfs_access,
    .xFullPathname = ram_vfs_fullpathname,
    .xDlOpen = ram_vfs_dlopen,
    .xDlError = ram_vfs_dlerror,
    .xDlSym = ram_vfs_dlsym,
    .xDlClose = ram_vfs_dlclose,
    .xRandomness = ram_vfs_randomness,
    .xSleep = ram_vfs_sleep,
    .xCurrentTime = ram_vfs_currenttime,
    .xGetLastError = ram_vfs_get_last_error,
    .xCurrentTimeInt64 = ram_vfs_currenttime_int64,
    .xSetSystemCall = ram_vfs_set_system_call,
    .xGetSystemCall = ram_vfs_get_system_call,
    .xNextSystemCall = ram_vfs_next_system_call,
};

int sqlite3_os_init(void)
{
    printf("[SQLite] Registering RAM VFS (64KB static buffer)...\r\n");
    int ret = sqlite3_vfs_register(&ram_vfs, 1);
    printf("[SQLite] Register result: %d\r\n", ret);
    return ret;
}

int sqlite3_os_end(void)
{
    return SQLITE_OK;
}

int sqlite_task_init(void)
{
    int result;

    printf("[SQLite] Step 1: Initializing SQLite OS layer...\r\n");
    result = sqlite3_os_init();
    if(result != SQLITE_OK)
    {
        printf("[SQLite] ERROR: sqlite3_os_init() failed with code: %d!\r\n", result);
        return -1;
    }
    printf("[SQLite] Step 1: PASSED\r\n");

    printf("[SQLite] Step 2: Opening database 'sensor.db' with RAM VFS...\r\n");
    result = sqlite3_open_v2("sensor.db", &db,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                            "ram_vfs");
    printf("[SQLite] sqlite3_open_v2 result: %d\r\n", result);
    if(result != SQLITE_OK)
    {
        printf("[SQLite] ERROR: sqlite3_open_v2() failed with code: %d\r\n", result);
        const char *err = sqlite3_errmsg(db);
        if(err != NULL)
        {
            printf("[SQLite] Error message: %s\r\n", err);
        }
        if(db != NULL)
        {
            sqlite3_close(db);
            db = NULL;
        }
        return -1;
    }
    printf("[SQLite] Step 2: PASSED - Database opened successfully\r\n");

    printf("[SQLite] Step 2.1: Configuring database pragmas...\r\n");
    char *err_msg = NULL;
    
    result = sqlite3_exec(db, "PRAGMA journal_mode=DELETE", NULL, NULL, &err_msg);
    if(result != SQLITE_OK)
    {
        printf("[SQLite] WARNING: Failed to set journal mode: %s\r\n", err_msg ? err_msg : "unknown");
        if(err_msg) sqlite3_free(err_msg);
    }
    else
    {
        printf("[SQLite] Journal mode set to DELETE\r\n");
    }
    
    result = sqlite3_exec(db, "PRAGMA synchronous=FULL", NULL, NULL, &err_msg);
    if(result != SQLITE_OK)
    {
        printf("[SQLite] WARNING: Failed to set synchronous: %s\r\n", err_msg ? err_msg : "unknown");
        if(err_msg) sqlite3_free(err_msg);
    }
    else
    {
        printf("[SQLite] Synchronous mode set to FULL\r\n");
    }
    
    result = sqlite3_exec(db, "PRAGMA locking_mode=NORMAL", NULL, NULL, &err_msg);
    if(result != SQLITE_OK)
    {
        printf("[SQLite] WARNING: Failed to set locking mode: %s\r\n", err_msg ? err_msg : "unknown");
        if(err_msg) sqlite3_free(err_msg);
    }
    else
    {
        printf("[SQLite] Locking mode set to NORMAL\r\n");
    }

    printf("[SQLite] Step 3: Creating table 'sensor_data'...\r\n");
    const char *sql = "CREATE TABLE IF NOT EXISTS sensor_data ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "timestamp INTEGER NOT NULL, "
                      "temperature REAL, "
                      "humidity REAL);";

    printf("[SQLite] About to execute CREATE TABLE SQL...\r\n");
    result = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    printf("[SQLite] sqlite3_exec result: %d\r\n", result);
    
    if(result != SQLITE_OK)
    {
        printf("[SQLite] ERROR: CREATE TABLE failed!\r\n");
        if(err_msg != NULL)
        {
            printf("[SQLite] Error message: %s\r\n", err_msg);
            sqlite3_free(err_msg);
        }
        const char *err = sqlite3_errmsg(db);
        if(err != NULL)
        {
            printf("[SQLite] Extended error: %s\r\n", err);
        }
        sqlite3_close(db);
        db = NULL;
        return -1;
    }
    printf("[SQLite] Step 3: PASSED - Table created successfully\r\n");
    printf("[SQLite] Initialization completed successfully!\r\n");

    return 0;
}

int sqlite_task_insert(float temperature, float humidity)
{
    if(db == NULL)
    {
        printf("[SQLite] ERROR: Database not initialized in sqlite_task_insert()\r\n");
        return -1;
    }

    char sql[128];
    int64_t timestamp = HAL_GetTick();

    snprintf(sql, sizeof(sql),
             "INSERT INTO sensor_data (timestamp, temperature, humidity) VALUES (%lld, %.2f, %.2f);",
             (long long)timestamp, temperature, humidity);

    printf("[SQLite] Inserting: temp=%.2f, humi=%.2f, ts=%lld\r\n", temperature, humidity, (long long)timestamp);

    char *err_msg = NULL;
    int result = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if(result != SQLITE_OK)
    {
        printf("[SQLite] ERROR: INSERT failed!\r\n");
        if(err_msg != NULL)
        {
            printf("[SQLite] Error message: %s\r\n", err_msg);
            sqlite3_free(err_msg);
        }
        return -1;
    }

    printf("[SQLite] Insert: SUCCESS\r\n");
    return 0;
}

int sqlite_task_query_latest(float *temperature, float *humidity)
{
    if(db == NULL)
    {
        printf("[SQLite] ERROR: Database not initialized in sqlite_task_query_latest()\r\n");
        return -1;
    }

    printf("[SQLite] Querying latest sensor data...\r\n");

    const char *sql = "SELECT temperature, humidity FROM sensor_data ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmt;
    int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if(result != SQLITE_OK)
    {
        printf("[SQLite] ERROR: sqlite3_prepare_v2() failed with code: %d\r\n", result);
        return -1;
    }

    result = sqlite3_step(stmt);
    if(result == SQLITE_ROW)
    {
        *temperature = (float)sqlite3_column_double(stmt, 0);
        *humidity = (float)sqlite3_column_double(stmt, 1);
        sqlite3_finalize(stmt);
        printf("[SQLite] Query Result: temp=%.2f, humi=%.2f\r\n", *temperature, *humidity);
        return 0;
    }
    else if(result == SQLITE_DONE)
    {
        printf("[SQLite] Query: No data found in database\r\n");
    }
    else
    {
        printf("[SQLite] ERROR: sqlite3_step() failed with code: %d\r\n", result);
    }

    sqlite3_finalize(stmt);
    return -1;
}

void sqlite_task_print_status(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("         SQLite Database Status        \r\n");
    printf("========================================\r\n");

    if(db == NULL)
    {
        printf("[DB] Database: NOT INITIALIZED\r\n");
    }
    else
    {
        const char *sql = "SELECT COUNT(*) FROM sensor_data";
        sqlite3_stmt *stmt;
        int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

        if(result != SQLITE_OK)
        {
            printf("[DB] Database: ERROR\r\n");
        }
        else
        {
            if(sqlite3_step(stmt) == SQLITE_ROW)
            {
                int count = sqlite3_column_int(stmt, 0);
                printf("[DB] Database: READY\r\n");
                printf("[DB] Records: %d\r\n", count);

                if(count > 0)
                {
                    sqlite3_finalize(stmt);

                    const char *sql2 = "SELECT timestamp, temperature, humidity FROM sensor_data ORDER BY id DESC LIMIT 1";
                    sqlite3_stmt *stmt2;
                    if(sqlite3_prepare_v2(db, sql2, -1, &stmt2, NULL) == SQLITE_OK)
                    {
                        if(sqlite3_step(stmt2) == SQLITE_ROW)
                        {
                            int ts = sqlite3_column_int(stmt2, 0);
                            float temp = (float)sqlite3_column_double(stmt2, 1);
                            float humi = (float)sqlite3_column_double(stmt2, 2);
                            printf("[DB] Latest: T=%dms | Temp=%.1fC | Humi=%.1f%%\r\n", ts, temp, humi);
                        }
                        sqlite3_finalize(stmt2);
                    }
                }
                else
                {
                    sqlite3_finalize(stmt);
                    printf("[DB] Database: EMPTY\r\n");
                }
            }
            else
            {
                sqlite3_finalize(stmt);
                printf("[DB] Database: EMPTY\r\n");
            }
        }
    }

    printf("========================================\r\n");
}

void vTaskSQLite(void *pvParameters)
{
    (void)pvParameters;
    int loop_count = 0;

    printf("[SQLite Task] Task started! Loop test begins...\r\n");
    
    while(1)
    {
        loop_count++;
        printf("[SQLite Task] Loop count: %d\r\n", loop_count);
        
        if(loop_count >= 3)
        {
            printf("[SQLite Task] Loop test complete. Proceeding to init.\r\n");
            break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("[SQLite Task] Step 1: Clearing RAM database storage (%dKB)...\r\n", RAM_DB_SIZE / 1024);
    memset(g_ram_db_storage, 0, RAM_DB_SIZE);
    printf("[SQLite Task] RAM storage cleared\r\n");

    printf("[SQLite Task] Step 2: Initializing SQLite Database...\r\n");
    if(sqlite_task_init() != 0)
    {
        printf("[SQLite Task] FATAL: SQLite initialization failed! Entering infinite loop with debug.\r\n");
        
        while(1)
        {
            loop_count++;
            printf("[SQLite Task] ERROR LOOP: Count=%d - SQLite init failed, waiting...\r\n", loop_count);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    printf("[SQLite Task] SQLite initialized successfully\r\n");

    printf("[SQLite Task] Step 3: Inserting 5 sensor data records...\r\n");
    for(int i = 0; i < 5; i++)
    {
        printf("[SQLite Task] Inserting record %d/5...\r\n", i + 1);
        if(sqlite_task_insert(25.0f + i * 0.5f, 60.0f + i * 2.0f) != 0)
        {
            printf("[SQLite Task] WARNING: Insert failed for record %d\r\n", i + 1);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("[SQLite Task] All records inserted\r\n");

    printf("[SQLite Task] Step 4: Querying latest record...\r\n");
    float temp, humi;
    if(sqlite_task_query_latest(&temp, &humi) == 0)
    {
        printf("[SQLite Task] Query SUCCESS: Latest data - Temp=%.2f, Humi=%.2f\r\n", temp, humi);
    }
    else
    {
        printf("[SQLite Task] Query FAILED or no data available\r\n");
    }

    printf("[SQLite Task] All operations completed. Task entering suspended state.\r\n");
    vTaskDelay(portMAX_DELAY);
}
