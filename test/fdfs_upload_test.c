/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "fdfs_api.h"

int fdfs_upload_by_filename1(const char *filename, char *file_id){
	
	// 需要管道
    int fd[2];
    int ret = pipe(fd);
	if(ret == -1){
		perror("pipe");
		return -1;	
	}
	
	// 创建子进程
	pid_t pid = fork();
	if(pid == -1)
	{
		perror("fork");
		return -1;
	}

	if(pid == 0)
	{
		// 子进程执行另外一个程序
		//printf("this is func +++++++++ execl +++++++++\n");
		//execl("/usr/bin/fdfs_upload_file", "fdfs_upload_file", NULL);
		printf("~~~~~~~~\n");
		close(fd[0]);
        // 数据写到管道，STDOUT_FILENO 指向管道的写端
        int ret = dup2(fd[1], STDOUT_FILENO);
		
		if(ret == 1){
			printf("~~~~~~~~%d", ret);
			perror("STDOUT_FILENO dup2");	
		}
		
		ret = dup2(stdout, fd[1]);
		
		if(ret == -1){
			printf("~~~~~~~~%d", ret);
			perror("stdout dup2");	
		}

		printf("~~~~~~~~");
        execl("/usr/bin/fdfs_upload_file","fdfs_upload_file", "/etc/fdfs/client.conf", filename, NULL);
		printf("~~~~~~~~~");
		// 如果执行成功没有返回值
        perror("execl fdfs_upload_file");
		return -1;

	} else {
		// 父进程  读管道,关闭写端
		close(fd[1]);
        //dup2(fd[0], STDIN_FILENO);
		wait(NULL);
		
		char buf[1024] = {0};
        read(fd[0], buf, sizeof(buf));
        printf("buf = %s\n", buf);
        close(fd[1]);
		
		strcpy(file_id, buf);
		return 0;
  
	}
}

/* -------------------------------------------*/
/**
 * @brief  fdfs_upload_by_filename
 *
 * @param filename 要上传文件路径
 * @param file_id 得到已经上传成功的fileid
 *
 * @returns   
 *       0 succ, -1 fail
 */
/* -------------------------------------------*/
int fdfs_upload_by_filename(const char *filename, char *file_id)
{
	const char *local_filename;
	char group_name[FDFS_GROUP_NAME_MAX_LEN + 1];
	ConnectionInfo *pTrackerServer;
	int result;
	int store_path_index;
	ConnectionInfo storageServer;

	log_init();
	g_log_context.log_level = LOG_ERR;
	ignore_signal_pipe();

    //加载一些配置文件
	if ((result=fdfs_client_init("/etc/fdfs/client.conf")) != 0)
	{
		return result;
	}

    //通过配置问价拿到内容 得到一个tracker链接的句柄
	pTrackerServer = tracker_get_connection();
	if (pTrackerServer == NULL)
	{
		fdfs_client_destroy();
		return errno != 0 ? errno : ECONNREFUSED;
	}

	local_filename = filename;
	*group_name = '\0';

    //通过询问tracker句柄 得到一个可用的storage句柄
	if ((result=tracker_query_storage_store(pTrackerServer, \
	                &storageServer, group_name, &store_path_index)) != 0)
	{
		fdfs_client_destroy();
		fprintf(stderr, "tracker_query_storage fail, " \
			"error no: %d, error info: %s\n", \
			result, STRERROR(result));
		return result;
	}

    //根据得到的storage句柄 上传本地local_filename
	result = storage_upload_by_filename1(pTrackerServer, \
			&storageServer, store_path_index, \
			local_filename, NULL, \
			NULL, 0, group_name, file_id);
	if (result == 0)
	{
        //上传succ
		printf("%s\n", file_id);
	}
	else
	{
		fprintf(stderr, "upload file fail, " \
			"error no: %d, error info: %s\n", \
			result, STRERROR(result));
	}

    //断开链接 回收资源
	tracker_disconnect_server_ex(pTrackerServer, true);
    
	fdfs_client_destroy();

	return result;
}

