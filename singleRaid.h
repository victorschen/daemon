#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "global.h"
#ifndef  _SINGLERAID_H_
#define  _SINGLERAID_H_

class SingleRaid {
public:
	Disk raidDisks[20];										// ������
	Disk spareDisks[8];										// ������
	int raidDiskNum;										// �����̵ĸ���
	int spareDiskNum;										// �����̵ĸ���
	int popedomNum;											// ������Ȩ�޵Ľڵ���Ŀ
	char mappingNo[5];										// ӳ���ͨ����
	char level[10];											// ���еļ���
	char chunk[5];											// ���ڴ�С
	char index[5];											// ���е����
	char devName[10];										// �豸��
	char popedom[30][20];									// Ȩ������
	SingleRaid();											// Ĭ�Ϲ��캯��
	void setAttribute(char * i,char * l,char * c,char * m);	// ��������ֵ
	void addRaidDisk(Disk& d);								// ���������
	void addSpareDisk(Disk& d);								// ��ӱ�����
	void create();											// ����������
	void start();											// ��������
	void stop();											// ֹͣ����
	void hotAddDisk(Disk& d);								// ����������Ӵ���
	void hotRemoveDisk(Disk& d);							// ���̴�������ɾ��
	void mapping();											// ӳ��
	void unmapping();										// ���ӳ��
	void encrypt();											// ����
	void addPopedom(char * name);							// ���Ȩ��
	void grow();											// ����
	bool operator==(SingleRaid& sr);
	int  getRaidDiskIndex(Disk& d);         				// ��ȡ�����������������е����
	int  getSpareDiskIndex(Disk& d);       					// ��ȡ�������ڱ��������е����
	void setMappingNo();									// ����ӳ���ͨ����
};

SingleRaid::SingleRaid()
{
	raidDiskNum = 0;
	spareDiskNum = 0;
	popedomNum = 0;
	strcpy(mappingNo,"");
	strcpy(level,"");
	strcpy(chunk,"");
	strcpy(index,"");
	strcpy(devName,"");
}

void SingleRaid::setAttribute(char * i,char * l,char * c,char * m)
{
	strcpy(index,i);
	strcpy(level,l);
	strcpy(chunk,c);
	// �����е������Ϊ���豸��
	strcpy(devName,"/dev/md");
	strcat(devName,index);
	strcpy(mappingNo,m);
	raidDiskNum = 0;
	spareDiskNum = 0;
	// ����ͨ����
	setMappingNo();
}

void SingleRaid::addRaidDisk(Disk& d)
{
	raidDisks[raidDiskNum] = d;
	raidDiskNum++;
}

void SingleRaid::addSpareDisk(Disk& d)
{
	spareDisks[spareDiskNum] = d;
	spareDiskNum++;
}

void SingleRaid::create()
{
	char cmdline[200],temp[5];
	strcpy(cmdline,"mdadm -C ");
	strcat(cmdline,devName);
	strcat(cmdline," --run");
	strcat(cmdline," -l");
	strcat(cmdline,level);
	strcat(cmdline," -n");
	gcvt(raidDiskNum,5,temp);
	strcat(cmdline,temp);
	if(strcmp(level,"0"))		// raid0��֧��spare disk
	{
		strcat(cmdline," -x");
		gcvt(spareDiskNum,5,temp);
		strcat(cmdline,temp);
	}
	strcat(cmdline," -c");
	strcat(cmdline,chunk);
	char tmp[50]="DEVICE ";
	for(int i = 0;i < raidDiskNum;i++)
	{
		strcat(cmdline," ");
		strcat(cmdline,raidDisks[i].devName);
		strcat(cmdline," ");

		strcat(tmp,raidDisks[i].devName);
		strcat(tmp," ");
	}
	if(strcmp(level,"0"))
	{
		for(int i = 0;i < spareDiskNum;i++)
		{
			strcat(cmdline," ");
			strcat(cmdline,spareDisks[i].devName);
			strcat(cmdline," ");
		}
	}
	system(cmdline);

	char buffer[200];
	sprintf(buffer,"echo \"%s\" > /etc/mdadm.conf",tmp);
	system(buffer);
	

	system("mdadm -Ds >> /etc/mdadm.conf");
	// ӳ��
	mapping();
	
}

void SingleRaid::start()
{
	char cmdline[200];
	strcpy(cmdline,"mdadm -A ");
	strcat(cmdline,devName);
	for(int i = 0;i < raidDiskNum;i++)
	{
		strcat(cmdline," ");
		strcat(cmdline,raidDisks[i].devName);
		strcat(cmdline," ");
	}
	if(strcmp(level,"0"))
	{
		for(int i = 0;i < spareDiskNum;i++)
		{
			strcat(cmdline," ");
			strcat(cmdline,spareDisks[i].devName);
			strcat(cmdline," ");
		}
	}
	system(cmdline);
	// ӳ��
	mapping();
}

void SingleRaid::stop()
{
	// Ҫ��ֹͣ����ǰֹͣӳ��
	unmapping();
	char cmdline[200];
	strcpy(cmdline,"mdadm -S ");
	strcat(cmdline,devName);
	system(cmdline);
}

void SingleRaid::hotAddDisk(Disk& d)
{
	char cmdline[200];
	strcpy(d.isSpareDisk,"1");
	addSpareDisk(d);	// �����������Ϊ������
	strcpy(cmdline,"mdadm -a ");
	strcat(cmdline,devName);
	strcat(cmdline," ");
	strcat(cmdline,d.devName);
	system(cmdline);
}

void SingleRaid::hotRemoveDisk(Disk& d)
{
	char cmdline[200];
	strcpy(d.isSpareDisk,"0");
	// �Ƚ��ô��̴�������ɾ��
	for(int i = 0;i < raidDiskNum;i++)
	{
		if(raidDisks[i] == d)
		{
			while(i < raidDiskNum - 1)
			{
				raidDisks[i] = raidDisks[i + 1];
				i++;
			}
			raidDiskNum--;
			break;
		}
	}
	for(int i = 0;i < spareDiskNum;i++)
	{
		if(spareDisks[i] == d)
		{
			while(i < spareDiskNum - 1)
			{
				spareDisks[i] = spareDisks[i + 1];
				i++;
			}
			spareDiskNum--;
			break;
		}
	}
	// ����������̴��������Ƴ�
	strcpy(cmdline,"mdadm -f ");
	strcat(cmdline,devName);
	strcat(cmdline," ");
	strcat(cmdline,d.devName);
	system(cmdline);  // set-faulty
	strcpy(cmdline,"mdadm -r ");
	strcat(cmdline,devName);
	strcat(cmdline," ");
	strcat(cmdline,d.devName);
	system(cmdline);
}

void SingleRaid::mapping()
{
	// ��Ϊ�����д���һ��group
	char cmdline[200];
	strcpy(cmdline,"echo \"add_group group");
	strcat(cmdline,index);
	strcat(cmdline,"\" > /proc/scsi_tgt/scsi_tgt");
	system(cmdline);
	// �������豸
	strcpy(cmdline,"echo \"open HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,devName);
	strcat(cmdline,"\" > /proc/scsi_tgt/disk_fileio/disk_fileio");
	system(cmdline);
	// ����ӳ�䵽��ǰ������group
	strcpy(cmdline,"echo \"add HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,mappingNo);
	strcat(cmdline,"\" > /proc/scsi_tgt/groups/group");
	strcat(cmdline,index);
	strcat(cmdline,"/devices");
	system(cmdline);
	// ���÷���Ȩ��
	for(int i = 0;i < popedomNum;i++)
	{
		if(!strcmp(popedom[i],"all"))
		{
			// ����������˿����Ļ�
			/*strcpy(cmdline,"echo \"add HUSTRAID");
			strcat(cmdline,index);
			strcat(cmdline," ");
			strcat(cmdline,mappingNo);
			strcat(cmdline,"\" > /proc/scsi_tgt/groups/Default/devices");
			system(cmdline);*/
			strcpy(cmdline,"echo \"ALL ALL\">/etc/initiators.allow");
			system(cmdline);
			return ;
		}
		else
		{
			/*strcpy(cmdline,"echo \"add ");
			strcat(cmdline,popedom[i]);
			strcat(cmdline,"\" > /proc/scsi_tgt/groups/group");
			strcat(cmdline,index);
			strcat(cmdline,"/names");
			system(cmdline);*/
			if (i == 0)
			{
			
			strcpy(cmdline,"echo \"ALL ALL\" >/etc/initiators.deny");
			system(cmdline);
			}
			strcpy(cmdline,"echo \"ALL ");
			strcat(cmdline,popedom[i]);
			strcat(cmdline,"\">/etc/initiators.allow");
			system(cmdline);

		}
	}
}

void SingleRaid::unmapping()
{
	// ɾ��Ϊ�����д�����group
	char cmdline[200];
	strcpy(cmdline,"echo \"del_group group");
	strcat(cmdline,index);
	strcat(cmdline,"\" > /proc/scsi_tgt/scsi_tgt");
	system(cmdline);
	// �������ɾ��Default�µļ�¼
	strcpy(cmdline,"echo \"del HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,mappingNo);
	strcat(cmdline,"\" > /proc/scsi_tgt/groups/Default/devices");
	system(cmdline);
	// �ر������豸
	strcpy(cmdline,"echo \"close HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,devName);
	strcat(cmdline,"\" > /proc/scsi_tgt/disk_fileio/disk_fileio");
	system(cmdline);
}

void SingleRaid::encrypt()
{
}

void SingleRaid::addPopedom(char * name)
{
	strcpy(popedom[popedomNum],name);
	popedomNum++;
}

void SingleRaid::grow()
{
}

bool SingleRaid::operator==(SingleRaid& sr)
{
	if((!strcmp(index,sr.index)) && (!strcmp(level,sr.level)))
		return true;
	else
		return false;
}

int SingleRaid::getRaidDiskIndex(Disk& d)
{
	for(int i = 0;i < raidDiskNum;i++)
	{
		if(d == raidDisks[i])
			return i;
	}
	return -1;
}

int SingleRaid::getSpareDiskIndex(Disk& d)
{
	for(int i = 0;i < spareDiskNum;i++)
	{
		if(d == spareDisks[i])
			return i;
	}
	return -1;
}

void SingleRaid::setMappingNo()
{
	if(!strcmp(mappingNo,""))  // ���ӳ��ͨ����Ϊ�յĻ�������֮
	{
		// ���0��ͨ���ѱ�ռ������ͨ������Ϊ�����
		if(!access("mappingFlag",0))
		{
			strcpy(mappingNo,index);
		}
		else  // ���û��ռ������Ϊ0��������ռ�õı�־�ļ�mappingFlag
		{
			strcpy(mappingNo,"0");
			FILE * fp = fopen("mappingFlag","w");
			fclose(fp);
		}
	}
}

#endif
