#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
//#include <sqlite3ext.h>


int ts_callback(void* arg, int nr, char **values, char **CloName)
{
	int i = 0;
	FILE* fp = (FILE*)arg;
	char str[128];

	for(i=0; i<nr; i++)
	{
		bzero(str, 128);
		sprintf(str, "%s\t", values[i]);
		fwrite(str, 1, strlen(str), fp);
	}

	fwrite("\n", 1, strlen("\n"), fp);
	return 0;
}

int sqlite3_ts(void)
{
	char sql_cmd[128];
	sqlite3* db;
	FILE* fp;
	
	sqlite3_open("test.db",&db);
	bzero(sql_cmd, sizeof(sql_cmd));
	sprintf(sql_cmd, "%s", "CREATE TABLE IF NOT EXISTS tb1(id INTEGER PRIMARY KEY, data TEX)");
	sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	bzero(sql_cmd, sizeof(sql_cmd));
	sprintf(sql_cmd, "%s", "INSERT INTO tb1(data) VALUES(\"line1\")");
	sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	bzero(sql_cmd, sizeof(sql_cmd));
	sprintf(sql_cmd, "%s", "INSERT INTO tb1(data) VALUES(\"line2\")");
	sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);
	bzero(sql_cmd, sizeof(sql_cmd));
	sprintf(sql_cmd, "%s", "INSERT INTO tb1(data) VALUES(\"line3\")");
	sqlite3_exec(db, sql_cmd, NULL, NULL, NULL);

	bzero(sql_cmd, sizeof(sql_cmd));
	sprintf(sql_cmd, "%s", "SELECT * FROM tb1");

	fp = fopen("ts_file", "wb");
	sqlite3_exec(db, sql_cmd, ts_callback, fp, NULL);
	fclose(fp);

	sqlite3_close(db);
	return 0;
}

int main(void)
{
	sqlite3_ts();
	return 0;
}
