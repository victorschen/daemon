#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "global.h"
#ifndef  _SINGLERAID_H_
#define  _SINGLERAID_H_

class SingleRaid {
public:
	Disk raidDisks[20];										// 阵列盘
	Disk spareDisks[8];										// 镜像盘
	int raidDiskNum;										// 阵列盘的个数
	int spareDiskNum;										// 镜像盘的个数
	int popedomNum;											// 已设置权限的节点数目
	char mappingNo[5];										// 映射的通道号
	char level[10];											// 阵列的级别
	char chunk[5];											// 串口大小
	char index[5];											// 阵列的序号
	char devName[10];										// 设备名
	char popedom[30][20];									// 权限数组
	SingleRaid();											// 默认构造函数
	void setAttribute(char * i,char * l,char * c,char * m);	// 设置属性值
	void addRaidDisk(Disk& d);								// 添加阵列盘
	void addSpareDisk(Disk& d);								// 添加备份盘
	void create();											// 创建该阵列
	void start();											// 启动阵列
	void stop();											// 停止阵列
	void hotAddDisk(Disk& d);								// 向阵列中添加磁盘
	void hotRemoveDisk(Disk& d);							// 将盘从阵列中删除
	void mapping();											// 映射
	void unmapping();										// 解除映射
	void encrypt();											// 加密
	void addPopedom(char * name);							// 添加权限
	void grow();											// 扩容
	bool operator==(SingleRaid& sr);
	int  getRaidDiskIndex(Disk& d);         				// 获取阵列盘在阵列盘组中的序号
	int  getSpareDiskIndex(Disk& d);       					// 获取备份盘在备份盘组中的序号
	void setMappingNo();									// 设置映射的通道号
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
	// 将阵列的序号作为其设备号
	strcpy(devName,"/dev/md");
	strcat(devName,index);
	strcpy(mappingNo,m);
	raidDiskNum = 0;
	spareDiskNum = 0;
	// 设置通道号
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
	if(strcmp(level,"0"))		// raid0不支持spare disk
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
	// 映射
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
	// 映射
	mapping();
}

void SingleRaid::stop()
{
	// 要在停止阵列前停止映射
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
	addSpareDisk(d);	// 将加入的盘作为备份盘
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
	// 先将该磁盘从数组中删除
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
	// 调用命令将磁盘从阵列中移出
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
	// 先为该阵列创建一个group
	char cmdline[200];
	strcpy(cmdline,"echo \"add_group group");
	strcat(cmdline,index);
	strcat(cmdline,"\" > /proc/scsi_tgt/scsi_tgt");
	system(cmdline);
	// 打开阵列设备
	strcpy(cmdline,"echo \"open HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,devName);
	strcat(cmdline,"\" > /proc/scsi_tgt/disk_fileio/disk_fileio");
	system(cmdline);
	// 将其映射到先前创建的group
	strcpy(cmdline,"echo \"add HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,mappingNo);
	strcat(cmdline,"\" > /proc/scsi_tgt/groups/group");
	strcat(cmdline,index);
	strcat(cmdline,"/devices");
	system(cmdline);
	// 设置访问权限
	for(int i = 0;i < popedomNum;i++)
	{
		if(!strcmp(popedom[i],"all"))
		{
			// 如果让所有人看到的话
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
	// 删除为该阵列创建的group
	char cmdline[200];
	strcpy(cmdline,"echo \"del_group group");
	strcat(cmdline,index);
	strcat(cmdline,"\" > /proc/scsi_tgt/scsi_tgt");
	system(cmdline);
	// 如果有则删除Default下的记录
	strcpy(cmdline,"echo \"del HUSTRAID");
	strcat(cmdline,index);
	strcat(cmdline," ");
	strcat(cmdline,mappingNo);
	strcat(cmdline,"\" > /proc/scsi_tgt/groups/Default/devices");
	system(cmdline);
	// 关闭阵列设备
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
	if(!strcmp(mappingNo,""))  // 如果映射通道号为空的话就设置之
	{
		// 如果0号通道已被占用则将其通道号设为其序号
		if(!access("mappingFlag",0))
		{
			strcpy(mappingNo,index);
		}
		else  // 如果没被占用则设为0，并产生占用的标志文件mappingFlag
		{
			strcpy(mappingNo,"0");
			FILE * fp = fopen("mappingFlag","w");
			fclose(fp);
		}
	}
}

#endif
