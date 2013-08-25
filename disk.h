#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#ifndef  _DISK_H_
#define  _DISK_H_

class Disk {
public:
	char type[10];  					// ������� SCSI/SATA
	char status[2];   					// ����״̬ 0(����)/1(���滻)/2(�γ�����)
	char sn[30];   						// �������к�
	char vendor[30];    				// ���̵���������
	char capacity[20];  				// ���̵�����
	char devName[10];   				// ���̵��豸��
	char host[10];						// 
	char channel[5];					// 
	char scsiID[5];						// 
	char lun[5];						// 
	char isSpareDisk[2];  				// �Ƿ�Ϊ���б����� 0(����)/1(��)
	Disk();
	void setAttribute(char * device);  	// ���ô�������
	bool getSnByDevName();				// ͨ���豸����ȡ�������к�
	bool getVenByDevName();				// ͨ���豸����ȡ������������
	bool getCapByDevName();				// ͨ���豸����ȡ��������
	bool getScsiIDByDevName();			// ͨ���豸����ȡscsiID
	bool refreshStatus();				// ˢ�´��̵�״̬
	bool operator==(Disk& d);   		// �ж��������̶����Ƿ����
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
	// �����豸����ȡ���̵�����ֵ
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
	// ����ʹ��sg_inq�����ȡ�������к�
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
		sn[strlen(sn) - 1] = '\0';	// ȥ��ĩβ�Ļ��з�
		fclose(fp);
		system("rm -f sn");
		return true;
	}
	fclose(fp);
	// ����ʹ��hdparm -i�����ȡ�������к�
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
		sn[strlen(sn) - 1] = '\0';	// ȥ��ĩβ�Ļ��з�
		fclose(fp);
		system("rm -f sn");
		return true;
	}
	fclose(fp);
	// ��ȡ�������к�ʧ��
	system("rm -f sn");
	return false;
}

bool Disk::getVenByDevName()
{
	chdir(workingDir);
	char buffer[100];
	// ʹ��sg_inq�����ȡ���̵���������
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
		vendor[strlen(vendor) - 1] = '\0';	// ȥ��ĩβ�Ļ��з�
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
	// ʹ��sg_readcap�����ȡ���̵���������
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
		capacity[strlen(capacity) - 1] = '\0';	// ȥ��ĩβ�Ļ��з�
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
	// ����ʹ��sg_inq�������˶Դ������к�
	strcpy(buffer,"sg_inq ");
	strcat(buffer,devName);
	strcat(buffer," > status");
	system(buffer);
	FILE * fp = fopen("status","r");
	while(fgets(buffer,100,fp) != NULL)
	{
		if(strncmp(buffer," Unit serial number",19))
			continue;
		/* �������豸����ȡ�����к�����̵����к���Ƚϣ�
		   ��ͬ��˵������״̬����������ͬ��˵�������Ѿ�����
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
	// ����ʹ��hdparm -i����˶Դ������к�
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
	// �˶Դ������к�ʧ��
	system("rm -f status");
	/* ���̱��γ�������,�������޷���ȡ�������к� */
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
	// ����sg_map��sg_scan����
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
			if(strstr(buffer1,devName) == NULL)  // ƥ���豸��
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
