#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#ifndef  _DISK_H_
#define  _DISK_H_

class Disk {
public:
	char type[10];  					// 磁盘类别 SCSI/SATA
	char status[2];   					// 磁盘状态 0(正常)/1(被替换)/2(拔出或损坏)
	char sn[30];   						// 磁盘序列号
	char vendor[30];    				// 磁盘的生产厂商
	char capacity[20];  				// 磁盘的容量
	char devName[10];   				// 磁盘的设备名
	char host[10];						// 
	char channel[5];					// 
	char scsiID[5];						// 
	char lun[5];						// 
	char isSpareDisk[2];  				// 是否为阵列备份盘 0(不是)/1(是)
	Disk();
	void setAttribute(char * device);  	// 设置磁盘属性
	bool getSnByDevName();				// 通过设备名获取磁盘序列号
	bool getVenByDevName();				// 通过设备名获取磁盘生产厂商
	bool getCapByDevName();				// 通过设备名获取磁盘容量
	bool getScsiIDByDevName();			// 通过设备名获取scsiID
	bool refreshStatus();				// 刷新磁盘的状态
	bool operator==(Disk& d);   		// 判断两个磁盘对象是否相等
};

Disk::Disk()
{
	strcpy(type,"SCSI");
	strcpy(status,"0");
	strcpy(sn,"");
	strcpy(vendor,"");
	strcpy(capacity,"");
	strcpy(devName,"");
	strcpy(host,"");
	strcpy(channel,"");
	strcpy(scsiID,"");
	strcpy(lun,"");
	strcpy(isSpareDisk,"0");
}

void Disk::setAttribute(char * device)
{
	// 根据设备名获取磁盘的属性值
	strcpy(devName,device);
	getScsiIDByDevName();
	getSnByDevName();
	getVenByDevName();
	getCapByDevName();
}

bool Disk::getSnByDevName()
{
	chdir(workingDir);
	char buffer[100];
	// 尝试使用sg_inq命令获取磁盘序列号
	strcpy(buffer,"sg_inq ");
	strcat(buffer,devName);
	strcat(buffer," > sn");
	system(buffer);
	FILE * fp = fopen("sn","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer," Unit serial number",19))
			continue;
		strcpy(sn,buffer + 21);
		sn[strlen(sn) - 1] = '\0';	// 去掉末尾的换行符
		fclose(fp);
		system("rm -f sn");
		return true;
	}
	fclose(fp);
	// 尝试使用hdparm -i命令获取磁盘序列号
	strcpy(buffer,"hdparm -i ");
	strcat(buffer,devName);
	strcat(buffer," > sn");
	system(buffer);
	fp = fopen("sn","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		char * p = strstr(buffer,"SerialNo");
		if(p == NULL)
			continue;
		strcpy(sn,p + 9);
		sn[strlen(sn) - 1] = '\0';	// 去掉末尾的换行符
		fclose(fp);
		system("rm -f sn");
		return true;
	}
	fclose(fp);
	// 读取磁盘序列号失败
	system("rm -f sn");
	return false;
}

bool Disk::getVenByDevName()
{
	chdir(workingDir);
	char buffer[100];
	// 使用sg_inq命令获取磁盘的生产厂商
	strcpy(buffer,"sg_inq ");
	strcat(buffer,devName);
	strcat(buffer," > vendor");
	system(buffer);
	FILE * fp = fopen("vendor","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer," Vendor",7))
			continue;
		strcpy(vendor,buffer + 24);
		vendor[strlen(vendor) - 1] = '\0';	// 去掉末尾的换行符
		fclose(fp);
		system("rm -f vendor");
		return true;
	}
	fclose(fp);
	system("rm -f vendor");
	return false;
}

bool Disk::getCapByDevName()
{
	chdir(workingDir);
	char buffer[100];
	// 使用sg_readcap命令获取磁盘的生产厂商
	strcpy(buffer,"sg_readcap ");
	strcat(buffer,devName);
	strcat(buffer," > capacity");
	system(buffer);
	FILE * fp = fopen("capacity","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		char * p = strstr(buffer,"MiB");
		if(p == NULL)
			continue;
		strcpy(capacity,p + 5);
		capacity[strlen(capacity) - 1] = '\0';	// 去掉末尾的换行符
		fclose(fp);
		system("rm -f capacity");
		return true;
	}
	fclose(fp);
	system("rm -f capacity");
	return false;
}

bool Disk::refreshStatus()
{
	chdir(workingDir);
	char buffer[100];
	// 尝试使用sg_inq命令来核对磁盘序列号
	strcpy(buffer,"sg_inq ");
	strcat(buffer,devName);
	strcat(buffer," > status");
	system(buffer);
	FILE * fp = fopen("status","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer," Unit serial number",19))
			continue;
		/* 将根据设备名获取的序列号与磁盘的序列号相比较，
		   相同则说明磁盘状态正常，不相同则说明磁盘已经更换
		 */
		if(!strncmp(sn,buffer + 21,strlen(sn)))
			strcpy(status,"0");
		else
			strcpy(status,"1");
		fclose(fp);
		system("rm -f status");
		return true;
	}
	fclose(fp);
	// 尝试使用hdparm -i命令核对磁盘序列号
	strcpy(buffer,"hdparm -i ");
	strcat(buffer,devName);
	strcat(buffer," > status");
	system(buffer);
	fp = fopen("status","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		char * p = strstr(buffer,"SerialNo");
		if(p == NULL)
			continue;
		if(!strncmp(sn,p + 9,strlen(sn)))
			strcpy(status,"0");
		else
			strcpy(status,"1");
		fclose(fp);
		system("rm -f status");
		return true;
	}
	fclose(fp);
	// 核对磁盘序列号失败
	system("rm -f status");
	/* 磁盘被拔除或已损坏,或者是无法获取磁盘序列号 */
	strcpy(status,"2");
	return false;
}

bool Disk::getScsiIDByDevName()
{
	int i = devName[7] - 'a';
	char temp[3];
	gcvt(i,3,temp);
	strcpy(scsiID,temp);
	
	//
	/* 
	chdir(workingDir);
	// 利用sg_map和sg_scan命令
	system("sg_map > map");
	system("sg_scan > scan");
	if((!access("map",0))&&(!access("scan",0)))
	{
		char buffer1[100];
		char buffer2[100];
		FILE * fp1 = fopen("map","r");
		FILE * fp2 = fopen("scan","r");
		while(fgets(buffer1,100,fp1) != NULL)
		{
			if(strstr(buffer1,devName) == NULL)  // 匹配设备名
				continue;
			while(fgets(buffer2,100,fp2) != NULL)
			{
			}
		}
		fclose(fp1);
		fclose(fp2);
		system("rm -f map");
		system("rm -f scan");
	}
	*/
}

bool Disk::operator==(Disk& d)
{
	if(!strcmp(devName,d.devName) && !strcmp(sn,d.sn))
		return true;
	else
		return false;
}

#endif
