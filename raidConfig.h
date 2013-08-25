#include<iostream.h>
#include<string.h>
#include<stdio.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "disk.h"
#include "singleRaid.h"
#include "global.h"
#ifndef  _RAIDCONFIG_H_
#define  _RAIDCONFIG_H_

class RaidConfig {
public:
	Disk disks[32];
	SingleRaid singleRaids[16];
	int diskNum;
	int singleRaidNum;
	RaidConfig();
	void addDisk(Disk& d);
	void addSingleRaid(SingleRaid& sr);
	void removeDisk(Disk& d);
	void removeSingleRaid(SingleRaid& sr);
	void saveRcToXml(char * xmlFileName);
	void buildRcFromXml(char * xmlFileName);
	int  getDiskIndex(Disk& d);
	int  getSingleRaidIndex(SingleRaid& sr);
};

// RaidConfig����Ĺ��캯��
RaidConfig::RaidConfig()
{
	diskNum = 0;
	singleRaidNum = 0;
}

// ��RaidConfig���������һ�����̶���
void RaidConfig::addDisk(Disk& d)
{
	disks[diskNum] = d;
	diskNum++;
}

// ��RaidConfig���������һ�����ж���
void RaidConfig::addSingleRaid(SingleRaid& sr)
{
	singleRaids[singleRaidNum] = sr;
	singleRaidNum++;
}

// ��RaidConfig�������Ƴ�һ�����̶���
void RaidConfig::removeDisk(Disk& d)
{
	for(int i = 0;i < diskNum;i++)
	{
		 if(disks[i] == d)
		 {
		 	 while(i < diskNum - 1)
		 	 {
		 	 	 disks[i] = disks[i + 1];
		 	 	 i++;
		 	 }
		 	 diskNum--;
		 	 break;
		 }
	}
}

// ��RaidConfig�������Ƴ�һ�����ж���
void RaidConfig::removeSingleRaid(SingleRaid& sr)
{
	for(int i = 0;i < singleRaidNum;i++)
	{
		 if(singleRaids[i] == sr)
		 {
		 	 while(i < singleRaidNum - 1)
		 	 {
		 	 	 singleRaids[i] = singleRaids[i + 1];
		 	 	 i++;
		 	 }
		 	 singleRaidNum--;
		 	 break;
		 }
	}
}

// ��RaidConfig���󱣴�Ϊ��Ӧ��XML�ļ�����ʽ
void RaidConfig::saveRcToXml(char * xmlFileName)
{
	char temp[300];  // ��ʱ����
	// ��������һ���յ�dom����
	// ����һ����ʱ��xml�ļ���ͨ�����ļ�����dom����
	FILE * fp = fopen("temp.xml","w");
	fputs("<?xml version=\"1.0\" ?>\n",fp);
	fputs("<Rack>\n",fp);
	fputs("</Rack>\n",fp);
	fclose(fp);
	xmlDocPtr doc = xmlParseFile("temp.xml");
	system("rm -f temp.xml");
	// ���м�������ֵ
	int rackDiskNum = 0;  // ���м��µĴ�������
	int rackRaidNum = 0;  // ���м��µ���������
	int lastRaidIndex = 0;  // ��һ��Ҫ���������еĺ���
	xmlNodePtr cur = xmlDocGetRootElement(doc);
	// ��RaidConfig�����еĴ��̺�������Ϣ�����뵽doc��
	for(int i = 0;i < diskNum;i++)
	{
		xmlNodePtr newNode = xmlNewTextChild(cur,NULL,(xmlChar *)"Disk",NULL);
		xmlNewProp(newNode,(xmlChar *)"type",(xmlChar *)disks[i].type);
		xmlNewProp(newNode,(xmlChar *)"status",(xmlChar *)disks[i].status);
		xmlNewProp(newNode,(xmlChar *)"sn",(xmlChar *)disks[i].sn);
		xmlNewProp(newNode,(xmlChar *)"vendor",(xmlChar *)disks[i].vendor);
		xmlNewProp(newNode,(xmlChar *)"capacity",(xmlChar *)disks[i].capacity);
		xmlNewProp(newNode,(xmlChar *)"devName",(xmlChar *)disks[i].devName);
		xmlNewProp(newNode,(xmlChar *)"isSpareDisk",(xmlChar *)disks[i].isSpareDisk);
		xmlNewProp(newNode,(xmlChar *)"host",(xmlChar *)disks[i].host);
		xmlNewProp(newNode,(xmlChar *)"scsiID",(xmlChar *)disks[i].scsiID);
		xmlNewProp(newNode,(xmlChar *)"lun",(xmlChar *)disks[i].lun);
		xmlNewProp(newNode,(xmlChar *)"channel",(xmlChar *)disks[i].channel);
		rackDiskNum++;
	}
	for(int i = 0;i < singleRaidNum;i++)
	{
		xmlNodePtr newNode = xmlNewTextChild(cur,NULL,(xmlChar *)"singleRaid",NULL);
		xmlNewProp(newNode,(xmlChar *)"index",(xmlChar *)singleRaids[i].index);
		xmlNewProp(newNode,(xmlChar *)"level",(xmlChar *)singleRaids[i].level);
		xmlNewProp(newNode,(xmlChar *)"chunk",(xmlChar *)singleRaids[i].chunk);
		xmlNewProp(newNode,(xmlChar *)"devName",(xmlChar *)singleRaids[i].devName);
		xmlNewProp(newNode,(xmlChar *)"mappingNo",(xmlChar *)singleRaids[i].mappingNo);
		gcvt(singleRaids[i].raidDiskNum,300,temp);
		xmlNewProp(newNode,(xmlChar *)"raidDiskNum",(xmlChar *)temp);
		gcvt(singleRaids[i].spareDiskNum,300,temp);
		xmlNewProp(newNode,(xmlChar *)"spareDiskNum",(xmlChar *)temp);
		// �����е�Ȩ�ޱ��浽���нڵ��������
		strcpy(temp,"");
		for(int j = 0;j < singleRaids[i].popedomNum;j++)
		{
			strcat(temp,singleRaids[i].popedom[j]);
			strcat(temp,"-");  // �ָ���
		}
		xmlNewProp(newNode,(xmlChar *)"popedom",(xmlChar *)temp);
		for(int j = 0;j < singleRaids[i].raidDiskNum;j++)
		{
			xmlNodePtr childNode = xmlNewTextChild(newNode,NULL,(xmlChar *)"Disk",NULL);
			xmlNewProp(childNode,(xmlChar *)"type",(xmlChar *)singleRaids[i].raidDisks[j].type);
			xmlNewProp(childNode,(xmlChar *)"status",(xmlChar *)singleRaids[i].raidDisks[j].status);
			xmlNewProp(childNode,(xmlChar *)"sn",(xmlChar *)singleRaids[i].raidDisks[j].sn);
			xmlNewProp(childNode,(xmlChar *)"vendor",(xmlChar *)singleRaids[i].raidDisks[j].vendor);
			xmlNewProp(childNode,(xmlChar *)"capacity",(xmlChar *)singleRaids[i].raidDisks[j].capacity);
			xmlNewProp(childNode,(xmlChar *)"devName",(xmlChar *)singleRaids[i].raidDisks[j].devName);
			xmlNewProp(childNode,(xmlChar *)"isSpareDisk",(xmlChar *)singleRaids[i].raidDisks[j].isSpareDisk);
			xmlNewProp(childNode,(xmlChar *)"host",(xmlChar *)singleRaids[i].raidDisks[j].host);
			xmlNewProp(childNode,(xmlChar *)"scsiID",(xmlChar *)singleRaids[i].raidDisks[j].scsiID);
			xmlNewProp(childNode,(xmlChar *)"lun",(xmlChar *)singleRaids[i].raidDisks[j].lun);
			xmlNewProp(childNode,(xmlChar *)"channel",(xmlChar *)singleRaids[i].raidDisks[j].channel);
			rackDiskNum++;
		}
		for(int j = 0;j < singleRaids[i].spareDiskNum;j++)
		{
			xmlNodePtr childNode = xmlNewTextChild(newNode,NULL,(xmlChar *)"Disk",NULL);
			xmlNewProp(childNode,(xmlChar *)"type",(xmlChar *)singleRaids[i].spareDisks[j].type);
			xmlNewProp(childNode,(xmlChar *)"status",(xmlChar *)singleRaids[i].spareDisks[j].status);
			xmlNewProp(childNode,(xmlChar *)"sn",(xmlChar *)singleRaids[i].spareDisks[j].sn);
			xmlNewProp(childNode,(xmlChar *)"vendor",(xmlChar *)singleRaids[i].spareDisks[j].vendor);
			xmlNewProp(childNode,(xmlChar *)"capacity",(xmlChar *)singleRaids[i].spareDisks[j].capacity);
			xmlNewProp(childNode,(xmlChar *)"devName",(xmlChar *)singleRaids[i].spareDisks[j].devName);
			xmlNewProp(childNode,(xmlChar *)"isSpareDisk",(xmlChar *)singleRaids[i].spareDisks[j].isSpareDisk);
			xmlNewProp(childNode,(xmlChar *)"host",(xmlChar *)singleRaids[i].spareDisks[j].host);
			xmlNewProp(childNode,(xmlChar *)"scsiID",(xmlChar *)singleRaids[i].spareDisks[j].scsiID);
			xmlNewProp(childNode,(xmlChar *)"lun",(xmlChar *)singleRaids[i].spareDisks[j].lun);
			xmlNewProp(childNode,(xmlChar *)"channel",(xmlChar *)singleRaids[i].spareDisks[j].channel);
			rackDiskNum++;
		}
		rackRaidNum++;
		int index = atoi(singleRaids[i].index);
		if(index + 1 > lastRaidIndex)
			lastRaidIndex = index + 1;
	}
	// �����м����Լ��뵽���мܽڵ���
	gcvt(rackDiskNum,10,temp);
	xmlNewProp(cur,(xmlChar *)"diskNum",(xmlChar *)temp);
	gcvt(rackRaidNum,10,temp);
	xmlNewProp(cur,(xmlChar *)"raidNum",(xmlChar *)temp);
	gcvt(lastRaidIndex,10,temp);
	xmlNewProp(cur,(xmlChar *)"lastRaidIndex",(xmlChar *)temp);
	// ����xml�����ļ�raidConfig.xml
	xmlSaveFormatFile(xmlFileName,doc,1);
	xmlFreeDoc(doc);
}

// ��XML�ļ������Ӧ��RaidConfig����
void RaidConfig::buildRcFromXml(char * xmlFileName)
{
	chdir(workingDir);
	// ͨ����ǰ�������ļ�raidConfig.xml����dom����
	xmlDocPtr doc = xmlParseFile(xmlFileName);
	// ��dom�����е���Ϣ���뵽���ù������rc��
	xmlNodePtr cur = xmlDocGetRootElement(doc);
	cur = cur -> xmlChildrenNode;
	while(cur != NULL)
	{
		if(!xmlStrcmp(cur -> name,(const xmlChar *)"Disk"))
		{
			// ����һ���������̽ڵ���Ϣ�Ĵ��̶���
			Disk d;
			strcpy(d.type,(char *)xmlGetProp(cur,(xmlChar *)"type"));
			strcpy(d.status,(char *)xmlGetProp(cur,(xmlChar *)"status"));
			strcpy(d.sn,(char *)xmlGetProp(cur,(xmlChar *)"sn"));
			strcpy(d.vendor,(char *)xmlGetProp(cur,(xmlChar *)"vendor"));
			strcpy(d.capacity,(char *)xmlGetProp(cur,(xmlChar *)"capacity"));
			strcpy(d.devName,(char *)xmlGetProp(cur,(xmlChar *)"devName"));
			strcpy(d.isSpareDisk,(char *)xmlGetProp(cur,(xmlChar *)"isSpareDisk"));
			strcpy(d.host,(char *)xmlGetProp(cur,(xmlChar *)"host"));
			strcpy(d.scsiID,(char *)xmlGetProp(cur,(xmlChar *)"scsiID"));
			strcpy(d.lun,(char *)xmlGetProp(cur,(xmlChar *)"lun"));
			strcpy(d.channel,(char *)xmlGetProp(cur,(xmlChar *)"channel"));
			addDisk(d);
		}
		else if(!xmlStrcmp(cur -> name,(const xmlChar *)"singleRaid"))
		{
			// ����һ���������нڵ���Ϣ�����ж���
			char i[5];char l[10];char c[5];char m[5];char temp[300];
			strcpy(i,(char *)xmlGetProp(cur,(xmlChar *)"index"));
			strcpy(l,(char *)xmlGetProp(cur,(xmlChar *)"level"));
			strcpy(c,(char *)xmlGetProp(cur,(xmlChar *)"chunk"));
			strcpy(m,(char *)xmlGetProp(cur,(xmlChar *)"mappingNo"));
			SingleRaid sr;
			sr.setAttribute(i,l,c,m);
			// ������Ȩ�޵Ľڵ����Ȩ��������
			strcpy(temp,(char *)xmlGetProp(cur,(xmlChar *)"popedom"));
			char * p1 = temp;
			char * p2 = p1;
			char temp1[20];
			while(strcmp(p2,""))
			{
				strcpy(temp1,"");
				p1 = strstr(p2,"-");  // Ѱ�ҷָ���
				strncat(temp1,p2,p1 - p2);
				sr.addPopedom(temp1);
				p2 = p1 + 1;  // �����ָ�������һ���ַ�
			}
			xmlNodePtr childCur = cur -> xmlChildrenNode;
			while(childCur != NULL)
			{
				// �������������´�����Ϣ�Ĵ��̶���
				Disk d;
				strcpy(d.type,(char *)xmlGetProp(childCur,(xmlChar *)"type"));
				strcpy(d.status,(char *)xmlGetProp(childCur,(xmlChar *)"status"));
				strcpy(d.sn,(char *)xmlGetProp(childCur,(xmlChar *)"sn"));
				strcpy(d.vendor,(char *)xmlGetProp(childCur,(xmlChar *)"vendor"));
				strcpy(d.capacity,(char *)xmlGetProp(childCur,(xmlChar *)"capacity"));
				strcpy(d.devName,(char *)xmlGetProp(childCur,(xmlChar *)"devName"));
				strcpy(d.isSpareDisk,(char *)xmlGetProp(childCur,(xmlChar *)"isSpareDisk"));
				strcpy(d.host,(char *)xmlGetProp(childCur,(xmlChar *)"host"));
				strcpy(d.scsiID,(char *)xmlGetProp(childCur,(xmlChar *)"scsiID"));
				strcpy(d.lun,(char *)xmlGetProp(childCur,(xmlChar *)"lun"));
				strcpy(d.channel,(char *)xmlGetProp(childCur,(xmlChar *)"channel"));
				if(!strcmp(d.isSpareDisk,"0"))
					sr.addRaidDisk(d);
				else
					sr.addSpareDisk(d);
				childCur = childCur -> next;
			}
			addSingleRaid(sr);
		}
		cur = cur -> next;
	}
	xmlFreeDoc(doc);
}

int RaidConfig::getDiskIndex(Disk& d)
{
	for(int i = 0;i < diskNum;i++)
	{
		if(disks[i] == d)
			return i;
	}
    return -1;
}

int RaidConfig::getSingleRaidIndex(SingleRaid& sr)
{
	for(int i = 0;i < singleRaidNum;i++)
	{
		if(singleRaids[i] == sr)
			return i;
	}
	return -1;
}

#endif
