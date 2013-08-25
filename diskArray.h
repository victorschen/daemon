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

// ���캯��
DiskArray::DiskArray()
{
	diskNum = 0;
}

// ͨ��fdisk�����ȡ������Ϣ����������
void DiskArray::fillArray()
{
	chdir(workingDir);
	char buffer[100];
	system("fdisk -l > disk");
	FILE * fp = fopen("disk","r");  // ͨ��fdisk�����ȡ��ǰ�����ϵĴ�����Ϣ
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

// ˢ�´�������
void DiskArray::refreshArray()
{
	for(int i = 0;i < diskNum;i++)
	{
		array[i].refreshStatus();
		/* ����״̬��0��ʾ����1��ʾ���кŲ�ƥ��2��ʾ���ܶ�ȡ���к� */
		if(!strcmp(array[i].status,"1"))  // ���´��̸����ɴ���
		{
			Disk d;
			d.setAttribute(array[i].devName);
			array[diskNum] = d;  // ���´��̼�������
			diskNum++;
		}
	}
}

// �������к�Ϊsn�Ĵ����������е��±꣬ʧ�ܷ���-1
int DiskArray::getIndexBySn(char * sn)
{
	for(int i = 0;i < diskNum;i++)
	{
		if(!strcmp(array[i].sn,sn))
			return i;
	}
	return -1;
}

// ɨ����뵽���мܵ����豸
void DiskArray::scanForNewDevice()
{
	chdir(workingDir);
	char buffer[100];
	system("fdisk -l > disk");
	FILE * fp = fopen("disk","r");  // ͨ��fdisk�����ȡ��ǰ�����ϵĴ�����Ϣ
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer,"Disk /dev/sd",12))
			continue;
		char devName[9] = "";
		strncpy(devName,buffer + 5,8);
		Disk d;
		d.setAttribute(devName);
		// ����ڴ���������û�д˴��̶���������Ϊ�µĴ��̶������
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

// �Ƴ�����������������Ϊindex�Ĵ��̶���
void DiskArray::removeDiskAt(int index)
{
	for(int i = index;i < diskNum - 1;i++)
	{
		array[i] = array[i + 1];
	}
	diskNum--;
}

#endif
