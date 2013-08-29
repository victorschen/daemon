#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<signal.h>
#include<sys/param.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<time.h>
#include "disk.h"
#include "diskArray.h"
#include "singleRaid.h"
#include "raidConfig.h"
#include "global.h"

// 记录Raid状态的数组,在WriteLog时用到
int raidStat[32];

void init_daemon(void)
{
	int pid;
	int i;
	if(pid = fork())
		exit(0);
	else if(pid < 0)
		exit(1);
	setsid();
	if(pid = fork())
		exit(0);
	else if(pid < 0) 
		exit(1);
	for(i = 0;i < NOFILE;++i)
		close(i);
	chdir("/");
	umask(0);
	return;
}

void printRcToFile(char * fileName,RaidConfig& rc)
{
	FILE * fp = fopen(fileName,"w");
	fputs("*******************************\n",fp);
	for(int i = 0;i < rc.diskNum;i++)
	{
		fprintf(fp,"DISK: %s | %s\n",rc.disks[i].devName,rc.disks[i].sn);
	}
	for(int i = 0;i < rc.singleRaidNum;i++)
	{
		fprintf(fp,"*******************************\n");
		fprintf(fp,"RAID: %s | %s | %s\n",rc.singleRaids[i].devName,rc.singleRaids[i].level,rc.singleRaids[i].mappingNo);
		for(int j = 0;j < rc.singleRaids[i].raidDiskNum;j++)
		{
			fprintf(fp,"RAIDDISK %s | %s\n",rc.singleRaids[i].raidDisks[j].devName,rc.singleRaids[i].raidDisks[j].sn);
		}
		for(int j = 0;j < rc.singleRaids[i].spareDiskNum;j++)
		{
			fprintf(fp,"SPAREDISK %s | %s\n",rc.singleRaids[i].spareDisks[j].devName,rc.singleRaids[i].spareDisks[j].sn);
		}
		fprintf(fp,"*******************************\n");
	}
	fclose(fp);
}

// 磁盘监控模块
void DiskMonitor(DiskArray& da,RaidConfig& rc)
{
	// 首先刷新da中的磁盘状态
	chdir(workingDir);
	da.refreshArray();
	for(int i = 0;i < da.diskNum;i++)
	{
		int isNewDisk = 1;  // 标志磁盘是否为新设备
		// 将磁盘与非阵列下的磁盘对比
		for(int j = 0;j < rc.diskNum;j++)
		{
			if(!strcmp(da.array[i].sn,rc.disks[j].sn))
			{
				strcpy(rc.disks[j].status,da.array[i].status);
				isNewDisk = 0;
				// 如果有磁盘状态不正常时的处理
				if(strcmp(rc.disks[j].status,"0"))
				{
					FILE * fp = fopen("broken_disk_evulsion_event","w");
					fclose(fp);
				}
				break;
			}
		}
		// 将磁盘与阵列下的磁盘对比
		for(int j = 0;j < rc.singleRaidNum;j++)
		{
			// 对比阵列盘组
			for(int k = 0;k < rc.singleRaids[j].raidDiskNum;k++)
			{
				if(!strcmp(da.array[i].sn,rc.singleRaids[j].raidDisks[k].sn))
				{
					strcpy(rc.singleRaids[j].raidDisks[k].status,da.array[i].status);
					isNewDisk = 0;
					// 如果磁盘状态不正常时的处理
					if(strcmp(rc.singleRaids[j].raidDisks[k].status,"0"))
					{
						FILE * fp = fopen("broken_disk_evulsion_event","w");
						fclose(fp);
					}
					break;
				}
			}
			// 对比备份盘组
			for(int k = 0;k < rc.singleRaids[j].spareDiskNum;k++)
			{
				if(!strcmp(rc.singleRaids[j].spareDisks[k].sn,da.array[i].sn))
				{
					strcpy(rc.singleRaids[j].spareDisks[k].status,da.array[i].status);
					isNewDisk = 0;
					// 如果磁盘状态不正常时的处理
					if(strcmp(rc.singleRaids[j].spareDisks[k].status,"0"))
					{
						FILE * fp = fopen("broken_disk_evulsion_event","w");
						fclose(fp);
					}
					break;
				}
			}
		}
		// 没有在阵列中发现该磁盘，说明是新添加的设备
		if(isNewDisk)
		{
			rc.addDisk(da.array[i]);
			// 创建标志文件
			FILE * fp = fopen("new_disk_insertion_event","w");
			fclose(fp);
		}
	}
	// 重新生成xml文件
	if(!access("raidConfig.xml",0))
		system("rm -f raidConfig.xml");
	rc.saveRcToXml("raidConfig.xml");
}

// 阵列配置更新模块
void RaidMonitor(RaidConfig& rc)
{
	chdir(workingDir);
	if(!access("raidConfigNew.xml",0))
	{
		RaidConfig rc1;
		rc1.buildRcFromXml("raidConfigNew.xml");
		printRcToFile("rc1",rc1);
		// 判断是否有新阵列创建
	    for(int i = 0;i < rc1.singleRaidNum;i++)
	    {
	    	if(rc.getSingleRaidIndex(rc1.singleRaids[i]) == -1)
	    	{
			FILE *fpt = fopen("daemonTest", "a"); //TEST
			fprintf(fpt,"====rc will add single raid===\n ====rc1 will create raid===="); //TEST
			
	    		rc.addSingleRaid(rc1.singleRaids[i]);
	    		rc1.singleRaids[i].create();

			fprintf(fpt,"addSingleRaid[%s]:", rc.singleRaids[i].devName); //TEST
			fclose(fpt); //TEST
	    	}
	    }
		// 判断是否有阵列被删除
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			if(rc1.getSingleRaidIndex(rc.singleRaids[i]) == -1)
			{
				rc.singleRaids[i].stop();
				// 如果此阵列的映射通道号为0则删除标志文件
				if(!strcmp(rc.singleRaids[i].mappingNo,"0"))
					system("rm -f mappingFlag");
				rc.removeSingleRaid(rc.singleRaids[i]);
				// 记下被删除的阵列的序号
				FILE * fp = fopen("RemovedRaids","a");
				fputs(rc.singleRaids[i].index,fp);
				fputs("\n",fp);
				fclose(fp);
			}
		}
		// 判断各阵列下的磁盘配置是否被修改
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			int j = rc1.getSingleRaidIndex(rc.singleRaids[i]);
			if(j != -1)
			{
				// 检查是否有磁盘移出
				for(int k = 0;k < rc.singleRaids[i].raidDiskNum;k++)
				{
					if(rc1.singleRaids[j].getRaidDiskIndex(rc.singleRaids[i].raidDisks[k]) == -1)
					{
						rc.singleRaids[i].hotRemoveDisk(rc.singleRaids[i].raidDisks[k]);
						k--;  // 移出了一个Disk，后面的Disk都向前移了一个
					}
				}
				for(int k = 0;k < rc.singleRaids[i].spareDiskNum;k++)
				{
					if(rc1.singleRaids[j].getSpareDiskIndex(rc.singleRaids[i].spareDisks[k]) == -1)
					{
						rc.singleRaids[i].hotRemoveDisk(rc.singleRaids[i].spareDisks[k]);
						k--;  // 移出了一个Disk，后面的Disk都向前移了一个
					}
				}
				// 检查是否有新盘加入
				for(int k = 0;k < rc1.singleRaids[j].raidDiskNum;k++)
				{
					if(rc.singleRaids[i].getRaidDiskIndex(rc1.singleRaids[j].raidDisks[k]) == -1)
					{
						rc.singleRaids[i].hotAddDisk(rc1.singleRaids[j].raidDisks[k]);
					}
				}
				for(int k = 0;k < rc1.singleRaids[j].spareDiskNum;k++)
				{
					if(rc.singleRaids[i].getSpareDiskIndex(rc1.singleRaids[j].spareDisks[k]) == -1)
					{
						rc.singleRaids[i].hotAddDisk(rc1.singleRaids[j].spareDisks[k]);
					}
				}
			}
		}
		// 检查空闲磁盘组
		for(int i = 0;i < rc.diskNum;i++)
		{
			if(rc1.getDiskIndex(rc.disks[i]) == -1)
			{
				rc.removeDisk(rc.disks[i]);
				i--;
			}
		}
		for(int i = 0;i < rc1.diskNum;i++)
		{
			if(rc.getDiskIndex(rc1.disks[i]) == -1)
			{
				rc.addDisk(rc1.disks[i]);
			}
		}
		// 重新生成xml文件
		system("rm -f raidConfigNew.xml");
		if(!access("raidConfig.xml",0))
			system("rm -f raidConfig.xml");
		printRcToFile("rc",rc);
		rc.saveRcToXml("raidConfig.xml");
	}
}

// 写系统日志模块
void WriteLog()
{
	int i,nLines;
	time_t t;
	struct tm * p;
	FILE * fp1, * fp2, * fp3, * fp4;
	char buffer1[100],buffer2[100],buffer3[100],buffer4[100],temp[100];
	chdir(workingDir);
	/* 查看是否有log文件，如果没有则创建它 */
	if(access("log",0))
	{
		fp1 = fopen("log","w");
		fclose(fp1);
	}
	/* 看是否有新的磁盘插入 */
	if(!access("new_disk_insertion_event",0))
	{
		system("rm -f new_disk_insertion_event");
		system("mv log log1");
		fp1 = fopen("log","w");  /* 创建一个新的日志文件 */
		fp2 = fopen("log1","r");  /* 打开原来的日志文件 */
		/* 先记下事件发生的时间 */
		time(&t);
		p = localtime(&t);
		fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
		fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
		/* 将有新磁盘插入这一事件记录入系统日志 */
		fputs("DISKADD|有新的磁盘插入，请刷新网页显示\n",fp1);
		/* 将以前的日志事件写回到新日志中[总条数控制在200条以内] */
		nLines = 0;
		while(fgets(buffer2,100,fp2) != NULL)
		{
			fputs(buffer2,fp1);
			nLines ++;
			if(nLines >= 199)
				break;
		}
		fclose(fp2);
		fclose(fp1);
		/* 删除原来的日志文件 */
		system("rm -f log1");
	}
	/* 看是否有坏的磁盘损坏或拔出 */
	if(!access("broken_disk_evulsion_event",0))
	{
		system("rm -f broken_disk_evulsion_event");
		system("mv log log1");
		fp1 = fopen("log","w");  /* 创建一个新的日志文件 */
		fp2 = fopen("log1","r");  /* 打开原来的日志文件 */
		/* 先记下事件发生的时间 */
		time(&t);
		p = localtime(&t);
		fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
		fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
		/* 将有新磁盘插入这一事件记录入系统日志 */
		fputs("DISKFAULTY|有磁盘损坏或拔出,请刷新网页显示\n",fp1);
		/* 将以前的日志事件写回到新日志中[总条数控制在200条以内] */
		nLines = 0;
		while(fgets(buffer2,100,fp2) != NULL)
		{
			fputs(buffer2,fp1);
			nLines ++;
			if(nLines >= 199)
				break;
		}
		fclose(fp2);
		fclose(fp1);
		/* 删除原来的日志文件 */
		system("rm -f log1");
	}
	/* 查看是否有阵列删除 */
	if(!access("RemovedRaids",0))
	{
		fp4 = fopen("RemovedRaids","r");
		while(fgets(buffer4,100,fp4) != NULL)
		{
			system("mv log log1");
			fp1 = fopen("log","w");  /* 创建一个新的日志文件 */
			fp2 = fopen("log1","r");  /* 打开原来的日志文件 */
			/* 先记下事件发生的时间 */
			time(&t);
			p = localtime(&t);
			fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
			fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
			/* 将有阵列被删除这一事件记录入系统日志 */
			fputs("DISKFAULTY|有阵列被删除，序号为：",fp1);
			fputs(buffer4,fp1);
			/* 将以前的日志事件写回到新日志中[总条数控制在200条以内] */
			nLines = 0;
			while(fgets(buffer2,100,fp2) != NULL)
			{
				fputs(buffer2,fp1);
				nLines ++;
				if(nLines >= 199)
					break;
			}
			fclose(fp2);
			fclose(fp1);
			/* 删除原来的日志文件 */
			system("rm -f log1");
		}
		fclose(fp4);
		/* 删除临时文件 */
		system("rm -f RemovedRaids");
	}
	/* 查看/proc/mdstat文件，监控磁盘阵列的运行状态 */
	system("mv log log1");
	fp1 = fopen("log","w");  /* 创建一个新的日志文件 */
	fp3 = fopen("/proc/raidstat","r");
	nLines = 0;
	while(fgets(buffer3,100,fp3) != NULL)
	{
		if(strncmp(buffer3,"raid",4))  /* 找到记录md信息的起始行 */
			continue;
		i = atoi(buffer3 + 4);  /* i为阵列的序号 */
		if(strstr(buffer3 + 1,"raid0") != NULL)  /* 如果阵列是raid0级别 */
		{
			if(strstr(buffer3,"(F)") != NULL)  /* raid0阵列中有磁盘损坏 */
			{
				if(raidStat[i] == 1)
					continue;  /* 说明上次已经报告了阵列的损坏状态，此次不用重复 */
				else
				{
					/* 先写入日志时间 */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|阵列");
					strncat(buffer1,buffer3 + 4,2);  // 加上阵列的序号
					strcat(buffer1,":有磁盘损坏或拔出，raid0阵列被损坏\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 1;
				}
			}
			else  /* raid0阵列处于正常工作状态 */
			{
				if(raidStat[i] == 0)
					continue;  /* 说明上次已经报告了阵列的正常状态，此次不用重复 */
				else
				{
					/* 先写入日志时间 */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|阵列");
					strncat(buffer1,buffer3 + 4,2);   // 加上阵列的序号
					strcat(buffer1,":状态正常\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 0;
				}
			}
		}
		else  /* 阵列非raid0级别[为raid1或raid5等] */
		{
			strcpy(temp,buffer3);  /* 保存buffer3的内容到temp中，主要为保存raid的序号 */
			fgets(buffer3,100,fp3);
			if(strstr(buffer3,"_") != NULL)  /* 有磁盘状态不正常，阵列处于降级或重建模式 */
			{
				fgets(buffer3,100,fp3);
				if(strstr(buffer3,"recovery") != NULL)  /* 阵列正在重建 */
				{
					if(((raidStat[i] % 2) == 0) && (raidStat[i] >= 2) && (raidStat[i] < 24))
					{
						raidStat[i] = raidStat[i] + 2;
						continue;
					}
					/* 将raid重建的进度写入日志 */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|阵列");
					strncat(buffer1,temp + 4,2);  // 加上阵列的序号
					strcat(buffer1,":正在重建 进度:");
					strncat(buffer1,strstr(buffer3,"recovery = ") + 11,5);
					strcat(buffer1," 剩余时间:");
					strncat(buffer1,strstr(buffer3,"finish=") + 7,strstr(buffer3,"min") - strstr(buffer3,"finish=") - 7);
					strcat(buffer1,"分\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 2;
				}
				else  /* 阵列处于降级模式 */
				{
					if(raidStat[i] == 1)
						continue;
					else
					{
						/* 先写入日志时间 */
						time(&t);
						p = localtime(&t);
						fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
						fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
						strcpy(buffer1,"RAIDSTATUS|阵列");
						strncat(buffer1,temp + 4,2);  // 加上阵列的序号
						strcat(buffer1,":处于降级模式\n");
						fputs(buffer1,fp1);
						nLines ++;
						raidStat[i] = 1;  /* 在数组中记下raid的当前降级状态 */
					}
				}
			}
			else  /* 阵列处于正常工作状态或正在同步 */
			{
				fgets(buffer3,100,fp3);
				if(strstr(buffer3,"resync") != NULL)  /* 阵列正在执行同步操作 */
				{
					if(((raidStat[i] % 2) == 1) && (raidStat[i] >= 3) && (raidStat[i] < 25))
					{
						raidStat[i] = raidStat[i] + 2;
						continue;
					}
					/* 将raid同步的进度写入日志 */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|阵列");
					strncat(buffer1,temp + 4,2);  // 加上阵列的序号
					strcat(buffer1,":正在同步 进度:");
					strncat(buffer1,strstr(buffer3,"resync = ") + 9,5);
					strcat(buffer1," 剩余时间:");
					strncat(buffer1,strstr(buffer3,"finish=") + 7,strstr(buffer3,"min") - strstr(buffer3,"finish=") - 7);
					strcat(buffer1,"分\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 3;
				}
				else  /* 阵列处于正常工作状态 */
				{
					if(raidStat[i] == 0)
						continue;
					else
					{
						/* 先写入日志时间 */
						time(&t);
						p = localtime(&t);
						fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
						fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
						strcpy(buffer1,"RAIDSTATUS|阵列");
						strncat(buffer1,temp + 4,2);  // 加上阵列的序号
						strcat(buffer1,":处于正常状态\n");
						fputs(buffer1,fp1);
						nLines ++;
						raidStat[i] = 0;  /* 在数组中记下raid的当前正常状态 */
					}
				}
			}
		}
	}
	fclose(fp3);  /* 关闭/proc/mdstat文件 */
	/* 将以前的日志事件写回到新日志中[总条数控制在200条以内] */
	fp2 = fopen("log1","r");  /* 打开原来的日志文件 */
	while(fgets(buffer2,100,fp2) != NULL)
	{
		fputs(buffer2,fp1);
		nLines ++;
		if(nLines >= 200)
			break;
	}
	fclose(fp2);
	fclose(fp1);
	/* 删掉原来的日志文件 */
	system("rm -f log1");
}

// 其他功能模块，重启监控和修改IP等
void Additional(RaidConfig& rc)
{
	FILE * fp1;
	char buffer1[100],cmdline1[100],cmdline2[100];
	chdir(workingDir);
	// 修改网络IP地址和DNS
	if(!access("ifcfg-eth0",0))
	{
		system("chown root:root ifcfg-eth0");
		system("mv -f ifcfg-eth0 /etc/sysconfig/network-scripts/ifcfg-eth0");
		fp1 = fopen("/etc/sysconfig/network-scripts/ifcfg-eth0","r");
		strcpy(cmdline1,"ifconfig eth0 ");
		strcpy(cmdline2,"route add default gw ");
		while(fgets(buffer1,100,fp1) != NULL)
		{
			if(strncmp(buffer1,"IPADDR",6))
				continue;
			strncat(cmdline1,buffer1 + 7,strlen(buffer1) - 8);  /* 读取IP地址 */
			strcat(cmdline1," netmask ");
			fgets(buffer1,100,fp1);  /* 读取子网掩码 */
			strncat(cmdline1,buffer1 + 8,strlen(buffer1) - 9);
			/* 读取默认网关 */
			fgets(buffer1,100,fp1);
			strncat(cmdline2,buffer1 + 8,strlen(buffer1) - 9);
		}
		system(cmdline1);  /* 修改IP地址和子网掩码 */
		system(cmdline2);  /* 修改默认网关 */
		fclose(fp1);
	}
	if(!access("resolv.conf",0))
	{
		system("chown root:root resolv.conf");
		system("mv -f resolv.conf /etc/resolv.conf");
	}
	// 关闭或重启监控
	if(!access("reboot_event",0))
	{
		// 停止所有正在运行的阵列
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			rc.singleRaids[i].stop();
		}
		system("rm -f reboot_event");
		system("reboot");
	}
	if(!access("halt_event",0))
	{
		// 停止所有正在运行的阵列
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			rc.singleRaids[i].stop();
		}
		system("rm -f halt_event");
		system("poweroff");
	}
}

main()
{
	init_daemon();
	chdir(workingDir);
	// 建立磁盘数组对象
	DiskArray da;
	da.fillArray();
	// 建立阵列配置管理对象
	RaidConfig rc;
	// 初始化raidStat数组
	for(int i = 0;i < 32;i++)
		raidStat[i] = -1;
	if(access("raidConfig.xml",0))  // 如果以前的xml配置文件不存在
	{
		for(int i = 0;i < da.diskNum;i++)
		{
			// 将磁盘对象信息加入配置管理对象rc中
			rc.addDisk(da.array[i]);
		}
		// 生成xml配置文件raidConfig.xml
		rc.saveRcToXml("raidConfig.xml");
	}
	else  // 如果已存在xml配置文件
	{
		// 通过raidConfig.xml文件填充rc对象
		rc.buildRcFromXml("raidConfig.xml");
		// 启动已经存在的阵列
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			rc.singleRaids[i].start();
		}
	}
	// 阵列配置文件raidConfig.xml和阵列配置管理对象rc生成完毕
	// 下面进入循环体
	while(1)
	{
		DiskMonitor(da,rc);
		RaidMonitor(rc);
		WriteLog();
		Additional(rc);
		sleep(10);
	}
}
