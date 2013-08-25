#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "global.h"
#ifndef  _DISKARRAY_H_
#define  _DISKARRAY_H_

class DiskArray {
public:
	Disk array[50];
	int diskNum;
	DiskArray();
	void fillArray();
	void refreshArray();
	int getIndexBySn(char * sn);
	void scanForNewDevice();
	void removeDiskAt(int index);
};

// 构造函数
DiskArray::DiskArray()
{
	diskNum = 0;
}

// 通过fdisk命令获取磁盘信息填充磁盘数组
void DiskArray::fillArray()
{
	chdir(workingDir);
	char buffer[100];
	system("fdisk -l > disk");
	FILE * fp = fopen("disk","r");  // 通过fdisk命令获取当前机器上的磁盘信息
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer,"Disk /dev/sd",12))
			continue;
		char devName[9] = "";
		strncpy(devName,buffer + 5,8);
		Disk d;
		d.setAttribute(devName);
		array[diskNum] = d;
		diskNum++;
	}
	fclose(fp);
	system("rm -f disk");
}

// 刷新磁盘数组
void DiskArray::refreshArray()
{
	for(int i = 0;i < diskNum;i++)
	{
		array[i].refreshStatus();
		/* 磁盘状态：0表示正常1表示序列号不匹配2表示不能读取序列号 */
		if(!strcmp(array[i].status,"1"))  // 有新磁盘更换旧磁盘
		{
			Disk d;
			d.setAttribute(array[i].devName);
			array[diskNum] = d;  // 将新磁盘加入数组
			diskNum++;
		}
	}
}

// 返回序列号为sn的磁盘在数组中的下标，失败返回-1
int DiskArray::getIndexBySn(char * sn)
{
	for(int i = 0;i < diskNum;i++)
	{
		if(!strcmp(array[i].sn,sn))
			return i;
	}
	return -1;
}

// 扫描插入到阵列架的新设备
void DiskArray::scanForNewDevice()
{
	chdir(workingDir);
	char buffer[100];
	system("fdisk -l > disk");
	FILE * fp = fopen("disk","r");  // 通过fdisk命令获取当前机器上的磁盘信息
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer,"Disk /dev/sd",12))
			continue;
		char devName[9] = "";
		strncpy(devName,buffer + 5,8);
		Disk d;
		d.setAttribute(devName);
		// 如果在磁盘数组中没有此磁盘对象则将其作为新的磁盘对象加入
		int isNewDevice = 1;
		for(int i = 0;i < diskNum;i++)
		{
			if(d == array[i])
			{
				isNewDevice = 0;
				break;
			}
		}
		if(isNewDevice)
		{
			array[diskNum] = d;
			diskNum++;
		}
	}
	fclose(fp);
	system("rm -f disk");
}

// 移出磁盘数组中索引号为index的磁盘对象
void DiskArray::removeDiskAt(int index)
{
	for(int i = index;i < diskNum - 1;i++)
	{
		array[i] = array[i + 1];
	}
	diskNum--;
}

#endif
