#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sqlite3.h>

#define RUNNING_STATE_DB					"/tmp/RunningState.db"
#define STATE_TABLE_INFO 					"CREATE TABLE IF NOT EXISTS state_tb(Id INTEGER PRIMARY KEY NOT NULL," \
											"String TEXT NOT NULL UNIQUE," \
											"Data TEXT NOT NULL," \
											"Date TIMESTAMP NOT NULL DEFAULT(datetime('now', 'localtime')))"
#define STATE_TABLE_NAME					"state_tb"
#define TEMPERATURE_TAP						"temperature"
#define LUX_TAP								"lux"

int waiting_lock_callback(void *ptr, int count)
{
	usleep(500*1000);
	printf("waiting for lock to release.\n");
	return 0;
}

int check_record_callback(void* arg, int nr, char **values, char **CloName)
{
	int* flag = (int*)arg;
	if(nr == 1)
	{
		*flag = 1;	//查询到数据 flag置1
	}
	return 0;
}

int database_write(char* databaseName, char* table, char* string, char* data)
{
	char sql_cmd[256];
	sqlite3* db;
	int rc;
	int checkFlag = 0;		//默认表中没有记录
	rc = sqlite3_open(databaseName, &db);
	if(rc)
	{
		perror("Open database fail: ");
		return -1;
	}
	else
	{	
		memset(sql_cmd, 0, sizeof(sql_cmd));
		sprintf(sql_cmd, "%s", STATE_TABLE_INFO);
		rc = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
		if(rc != SQLITE_OK)
		{
			perror("create table fail");
			sqlite3_close(db);
			return -2;
		}
		else
		{
			printf("create table success or table exists.\n");
			
			memset(sql_cmd, 0, sizeof(sql_cmd));
			sprintf(sql_cmd, "SELECT Data FROM %s WHERE String='%s';", table, string);
			sqlite3_busy_handler(db, waiting_lock_callback, NULL); //设置数据库并发处理机制,等待锁
			rc = sqlite3_exec(db, sql_cmd, check_record_callback, &checkFlag, NULL);
			if(rc != SQLITE_OK)
			{
				perror("check database fail");
				return -3;
			}
			else
			{
				memset(sql_cmd, 0, sizeof(sql_cmd));
				if(checkFlag == 0)				//表中没有记录 可增加
				{
//					sprintf(sql_cmd, "INSERT INTO %s(String, Data) VALUES('%s', '%s');", table, string, data);
					sprintf(sql_cmd, "INSERT INTO %s(String, Data, Date) VALUES('%s', '%s', datetime('now', 'localtime'));", table, string, data);
				}
				else if(checkFlag == 1)			//表中已有数据 只能修改
				{
//					sprintf(sql_cmd, "UPDATE %s SET Data=%s WHERE String='%s';", table, data, string);
					sprintf(sql_cmd, "UPDATE %s SET Data=%s,Date=datetime('now', 'localtime') WHERE String='%s';", table, data, string);
				}
				else
				{
					printf("Bad operation...\n");
					return -4;
				}
				sqlite3_busy_handler(db, waiting_lock_callback, NULL); //设置数据库并发处理机制,等待锁
				rc = sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
				if(rc != SQLITE_OK)
				{
					perror("write database fail:");
					sqlite3_close(db);
					return -5;
				}
				else
				{
					sqlite3_close(db);
					return 0;
				}
			}
		}
	}	
}

/*
int read_int_callback(void* arg, int nr, char **values, char **CloName)
{
	int i = 0;
	int temp = 0;
	temp = atoi(values[nr-1]);
	*(int*)arg = temp;
	return 0;
}
*/

int read_string_callback(void* arg, int nr, char **values, char **CloName)
{
	int i = 0;
	sprintf((char*)arg, "%s", values[0]);
	return 0;
}

int database_read(char* databaseName, char* table, char* string, char* data)
{
	char sql_cmd[256];
	sqlite3* db;
	int rc;
	
	rc = sqlite3_open(databaseName, &db);
	if(rc)
	{
		perror("Create database fail: ");
		return -1;
	}
	else
	{
		memset(sql_cmd, 0, sizeof(sql_cmd));
		sprintf(sql_cmd, "SELECT Data FROM %s WHERE String='%s';", table, string);
		sqlite3_busy_handler(db, waiting_lock_callback, NULL); //设置数据库并发处理机制,等待锁
		rc = sqlite3_exec(db, sql_cmd, read_string_callback, data, NULL);
		if(rc != SQLITE_OK)
		{
			perror("read the last temperature fail: ");
			sqlite3_close(db);
			return -2;
		}
		else
		{
			sqlite3_close(db);
			return 0;
		}
	}
}

int main(int argc, char* argv[])
{
	char temp_write[4];
	char temp_read[4];
	char lux_w[6];
	char lux_r[6];
	int i = 0;
	int ret = -3;
	struct timeval tv, tv2;
	
	
	if(argc == 3)
	{
		sprintf(temp_write, "%s", argv[1]);
		sprintf(lux_w, "%s", argv[2]);
	}
	else
	{
		printf("please input argv...");
		return -2;
	}
	
	gettimeofday(&tv, NULL);
	ret = database_write(RUNNING_STATE_DB, STATE_TABLE_NAME,
							 TEMPERATURE_TAP, temp_write);
	gettimeofday(&tv2, NULL);
	printf("1.1 time of write database = %ds\n", (tv2.tv_sec-tv.tv_sec));
	printf("1.2 time of write database = %dms\n", ((tv2.tv_sec-tv.tv_sec)*1000 + (tv2.tv_usec-tv.tv_usec)/1000));
	printf("1.3 time of write database = %dus\n", ((tv2.tv_sec-tv.tv_sec)*1000*1000 + (tv2.tv_usec-tv.tv_usec)));
	if(ret != 0)
	{
		printf("ERROR happendd when write temperature, ret=%d\n", ret);
		return -1;
	}

	ret = database_write(RUNNING_STATE_DB, STATE_TABLE_NAME,
							LUX_TAP, lux_w);
	if(ret != 0)
	{
		printf("ERROR happendd when write lux, ret=%d\n", ret);
		return -1;
	}

	gettimeofday(&tv, NULL);
	ret = database_read(RUNNING_STATE_DB, STATE_TABLE_NAME,
							TEMPERATURE_TAP, temp_read);
	gettimeofday(&tv2, NULL);
	printf("2.1 time of read database = %ds\n", (tv2.tv_sec-tv.tv_sec));
	printf("2.2 time of read database = %dms\n", ((tv2.tv_sec-tv.tv_sec)*1000 + (tv2.tv_usec-tv.tv_usec)/1000));
	printf("2.3 time of read database = %dus\n", ((tv2.tv_sec-tv.tv_sec)*1000*1000 + (tv2.tv_usec-tv.tv_usec)));
	
	if(ret != 0)
	{
		printf("ERROR happendd when read temperature, ret=%d\n", ret);
		return -2;
	}
	else
	{
		printf("the temperature=%s\n", temp_read);
	}

	ret = database_read(RUNNING_STATE_DB, STATE_TABLE_NAME,
							LUX_TAP, lux_r);
	if(ret != 0)
	{
		printf("ERROR happendd when read lux, ret=%d\n", ret);
		return -2;
	}
	else
	{
		printf("the lux=%s\n", lux_r);
	}
	
#if 1
	FILE* fp;
	char buf_w[] = "lux=4000";
	char buf_r[32];
	gettimeofday(&tv, NULL);
	fp = fopen("/tmp/test.txt", "wb");
	fwrite(buf_w, strlen(buf_w), 1, fp);
	fclose(fp);
	gettimeofday(&tv2, NULL);
	printf("3.1 time of write file = %ds\n", (tv2.tv_sec-tv.tv_sec));
	printf("3.2 time of write file = %dms\n", ((tv2.tv_sec-tv.tv_sec)*1000 + (tv2.tv_usec-tv.tv_usec)/1000));
	printf("3.3 time of write file = %dus\n", ((tv2.tv_sec-tv.tv_sec)*1000*1000 + (tv2.tv_usec-tv.tv_usec)));
	
	gettimeofday(&tv, NULL);
	fp = fopen("/tmp/test.txt", "rb");
	fread(buf_r, 1, sizeof(buf_r), fp);
	fclose(fp);
	gettimeofday(&tv2, NULL);
	printf("4.1 time of read file = %ds\n", (tv2.tv_sec-tv.tv_sec));
	printf("4.2 time of read file = %dms\n", ((tv2.tv_sec-tv.tv_sec)*1000 + (tv2.tv_usec-tv.tv_usec)/1000));
	printf("4.3 time of read file = %dus\n", ((tv2.tv_sec-tv.tv_sec)*1000*1000 + (tv2.tv_usec-tv.tv_usec)));
	printf("buf_r=%s\n", buf_r);
#endif
	
	return 0;
}
