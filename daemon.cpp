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

// ��¼Raid״̬������,��WriteLogʱ�õ�
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

// ���̼��ģ��
void DiskMonitor(DiskArray& da,RaidConfig& rc)
{
	// ����ˢ��da�еĴ���״̬
	chdir(workingDir);
	da.refreshArray();
	for(int i = 0;i < da.diskNum;i++)
	{
		int isNewDisk = 1;  // ��־�����Ƿ�Ϊ���豸
		// ��������������µĴ��̶Ա�
		for(int j = 0;j < rc.diskNum;j++)
		{
			if(!strcmp(da.array[i].sn,rc.disks[j].sn))
			{
				strcpy(rc.disks[j].status,da.array[i].status);
				isNewDisk = 0;
				// ����д���״̬������ʱ�Ĵ���
				if(strcmp(rc.disks[j].status,"0"))
				{
					FILE * fp = fopen("broken_disk_evulsion_event","w");
					fclose(fp);
				}
				break;
			}
		}
		// �������������µĴ��̶Ա�
		for(int j = 0;j < rc.singleRaidNum;j++)
		{
			// �Ա���������
			for(int k = 0;k < rc.singleRaids[j].raidDiskNum;k++)
			{
				if(!strcmp(da.array[i].sn,rc.singleRaids[j].raidDisks[k].sn))
				{
					strcpy(rc.singleRaids[j].raidDisks[k].status,da.array[i].status);
					isNewDisk = 0;
					// �������״̬������ʱ�Ĵ���
					if(strcmp(rc.singleRaids[j].raidDisks[k].status,"0"))
					{
						FILE * fp = fopen("broken_disk_evulsion_event","w");
						fclose(fp);
					}
					break;
				}
			}
			// �Աȱ�������
			for(int k = 0;k < rc.singleRaids[j].spareDiskNum;k++)
			{
				if(!strcmp(rc.singleRaids[j].spareDisks[k].sn,da.array[i].sn))
				{
					strcpy(rc.singleRaids[j].spareDisks[k].status,da.array[i].status);
					isNewDisk = 0;
					// �������״̬������ʱ�Ĵ���
					if(strcmp(rc.singleRaids[j].spareDisks[k].status,"0"))
					{
						FILE * fp = fopen("broken_disk_evulsion_event","w");
						fclose(fp);
					}
					break;
				}
			}
		}
		// û���������з��ָô��̣�˵��������ӵ��豸
		if(isNewDisk)
		{
			rc.addDisk(da.array[i]);
			// ������־�ļ�
			FILE * fp = fopen("new_disk_insertion_event","w");
			fclose(fp);
		}
	}
	// ��������xml�ļ�
	if(!access("raidConfig.xml",0))
		system("rm -f raidConfig.xml");
	rc.saveRcToXml("raidConfig.xml");
}

// �������ø���ģ��
void RaidMonitor(RaidConfig& rc)
{
	chdir(workingDir);
	if(!access("raidConfigNew.xml",0))
	{
		RaidConfig rc1;
		rc1.buildRcFromXml("raidConfigNew.xml");
		printRcToFile("rc1",rc1);
		// �ж��Ƿ��������д���
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
		// �ж��Ƿ������б�ɾ��
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			if(rc1.getSingleRaidIndex(rc.singleRaids[i]) == -1)
			{
				rc.singleRaids[i].stop();
				// ��������е�ӳ��ͨ����Ϊ0��ɾ����־�ļ�
				if(!strcmp(rc.singleRaids[i].mappingNo,"0"))
					system("rm -f mappingFlag");
				rc.removeSingleRaid(rc.singleRaids[i]);
				// ���±�ɾ�������е����
				FILE * fp = fopen("RemovedRaids","a");
				fputs(rc.singleRaids[i].index,fp);
				fputs("\n",fp);
				fclose(fp);
			}
		}
		// �жϸ������µĴ��������Ƿ��޸�
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			int j = rc1.getSingleRaidIndex(rc.singleRaids[i]);
			if(j != -1)
			{
				// ����Ƿ��д����Ƴ�
				for(int k = 0;k < rc.singleRaids[i].raidDiskNum;k++)
				{
					if(rc1.singleRaids[j].getRaidDiskIndex(rc.singleRaids[i].raidDisks[k]) == -1)
					{
						rc.singleRaids[i].hotRemoveDisk(rc.singleRaids[i].raidDisks[k]);
						k--;  // �Ƴ���һ��Disk�������Disk����ǰ����һ��
					}
				}
				for(int k = 0;k < rc.singleRaids[i].spareDiskNum;k++)
				{
					if(rc1.singleRaids[j].getSpareDiskIndex(rc.singleRaids[i].spareDisks[k]) == -1)
					{
						rc.singleRaids[i].hotRemoveDisk(rc.singleRaids[i].spareDisks[k]);
						k--;  // �Ƴ���һ��Disk�������Disk����ǰ����һ��
					}
				}
				// ����Ƿ������̼���
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
		// �����д�����
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
		// ��������xml�ļ�
		system("rm -f raidConfigNew.xml");
		if(!access("raidConfig.xml",0))
			system("rm -f raidConfig.xml");
		printRcToFile("rc",rc);
		rc.saveRcToXml("raidConfig.xml");
	}
}

// дϵͳ��־ģ��
void WriteLog()
{
	int i,nLines;
	time_t t;
	struct tm * p;
	FILE * fp1, * fp2, * fp3, * fp4;
	char buffer1[100],buffer2[100],buffer3[100],buffer4[100],temp[100];
	chdir(workingDir);
	/* �鿴�Ƿ���log�ļ������û���򴴽��� */
	if(access("log",0))
	{
		fp1 = fopen("log","w");
		fclose(fp1);
	}
	/* ���Ƿ����µĴ��̲��� */
	if(!access("new_disk_insertion_event",0))
	{
		system("rm -f new_disk_insertion_event");
		system("mv log log1");
		fp1 = fopen("log","w");  /* ����һ���µ���־�ļ� */
		fp2 = fopen("log1","r");  /* ��ԭ������־�ļ� */
		/* �ȼ����¼�������ʱ�� */
		time(&t);
		p = localtime(&t);
		fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
		fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
		/* �����´��̲�����һ�¼���¼��ϵͳ��־ */
		fputs("DISKADD|���µĴ��̲��룬��ˢ����ҳ��ʾ\n",fp1);
		/* ����ǰ����־�¼�д�ص�����־��[������������200������] */
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
		/* ɾ��ԭ������־�ļ� */
		system("rm -f log1");
	}
	/* ���Ƿ��л��Ĵ����𻵻�γ� */
	if(!access("broken_disk_evulsion_event",0))
	{
		system("rm -f broken_disk_evulsion_event");
		system("mv log log1");
		fp1 = fopen("log","w");  /* ����һ���µ���־�ļ� */
		fp2 = fopen("log1","r");  /* ��ԭ������־�ļ� */
		/* �ȼ����¼�������ʱ�� */
		time(&t);
		p = localtime(&t);
		fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
		fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
		/* �����´��̲�����һ�¼���¼��ϵͳ��־ */
		fputs("DISKFAULTY|�д����𻵻�γ�,��ˢ����ҳ��ʾ\n",fp1);
		/* ����ǰ����־�¼�д�ص�����־��[������������200������] */
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
		/* ɾ��ԭ������־�ļ� */
		system("rm -f log1");
	}
	/* �鿴�Ƿ�������ɾ�� */
	if(!access("RemovedRaids",0))
	{
		fp4 = fopen("RemovedRaids","r");
		while(fgets(buffer4,100,fp4) != NULL)
		{
			system("mv log log1");
			fp1 = fopen("log","w");  /* ����һ���µ���־�ļ� */
			fp2 = fopen("log1","r");  /* ��ԭ������־�ļ� */
			/* �ȼ����¼�������ʱ�� */
			time(&t);
			p = localtime(&t);
			fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
			fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
			/* �������б�ɾ����һ�¼���¼��ϵͳ��־ */
			fputs("DISKFAULTY|�����б�ɾ�������Ϊ��",fp1);
			fputs(buffer4,fp1);
			/* ����ǰ����־�¼�д�ص�����־��[������������200������] */
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
			/* ɾ��ԭ������־�ļ� */
			system("rm -f log1");
		}
		fclose(fp4);
		/* ɾ����ʱ�ļ� */
		system("rm -f RemovedRaids");
	}
	/* �鿴/proc/mdstat�ļ�����ش������е�����״̬ */
	system("mv log log1");
	fp1 = fopen("log","w");  /* ����һ���µ���־�ļ� */
	fp3 = fopen("/proc/raidstat","r");
	nLines = 0;
	while(fgets(buffer3,100,fp3) != NULL)
	{
		if(strncmp(buffer3,"raid",4))  /* �ҵ���¼md��Ϣ����ʼ�� */
			continue;
		i = atoi(buffer3 + 4);  /* iΪ���е���� */
		if(strstr(buffer3 + 1,"raid0") != NULL)  /* ���������raid0���� */
		{
			if(strstr(buffer3,"(F)") != NULL)  /* raid0�������д����� */
			{
				if(raidStat[i] == 1)
					continue;  /* ˵���ϴ��Ѿ����������е���״̬���˴β����ظ� */
				else
				{
					/* ��д����־ʱ�� */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|����");
					strncat(buffer1,buffer3 + 4,2);  // �������е����
					strcat(buffer1,":�д����𻵻�γ���raid0���б���\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 1;
				}
			}
			else  /* raid0���д�����������״̬ */
			{
				if(raidStat[i] == 0)
					continue;  /* ˵���ϴ��Ѿ����������е�����״̬���˴β����ظ� */
				else
				{
					/* ��д����־ʱ�� */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|����");
					strncat(buffer1,buffer3 + 4,2);   // �������е����
					strcat(buffer1,":״̬����\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 0;
				}
			}
		}
		else  /* ���з�raid0����[Ϊraid1��raid5��] */
		{
			strcpy(temp,buffer3);  /* ����buffer3�����ݵ�temp�У���ҪΪ����raid����� */
			fgets(buffer3,100,fp3);
			if(strstr(buffer3,"_") != NULL)  /* �д���״̬�����������д��ڽ������ؽ�ģʽ */
			{
				fgets(buffer3,100,fp3);
				if(strstr(buffer3,"recovery") != NULL)  /* ���������ؽ� */
				{
					if(((raidStat[i] % 2) == 0) && (raidStat[i] >= 2) && (raidStat[i] < 24))
					{
						raidStat[i] = raidStat[i] + 2;
						continue;
					}
					/* ��raid�ؽ��Ľ���д����־ */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|����");
					strncat(buffer1,temp + 4,2);  // �������е����
					strcat(buffer1,":�����ؽ� ����:");
					strncat(buffer1,strstr(buffer3,"recovery = ") + 11,5);
					strcat(buffer1," ʣ��ʱ��:");
					strncat(buffer1,strstr(buffer3,"finish=") + 7,strstr(buffer3,"min") - strstr(buffer3,"finish=") - 7);
					strcat(buffer1,"��\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 2;
				}
				else  /* ���д��ڽ���ģʽ */
				{
					if(raidStat[i] == 1)
						continue;
					else
					{
						/* ��д����־ʱ�� */
						time(&t);
						p = localtime(&t);
						fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
						fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
						strcpy(buffer1,"RAIDSTATUS|����");
						strncat(buffer1,temp + 4,2);  // �������е����
						strcat(buffer1,":���ڽ���ģʽ\n");
						fputs(buffer1,fp1);
						nLines ++;
						raidStat[i] = 1;  /* �������м���raid�ĵ�ǰ����״̬ */
					}
				}
			}
			else  /* ���д�����������״̬������ͬ�� */
			{
				fgets(buffer3,100,fp3);
				if(strstr(buffer3,"resync") != NULL)  /* ��������ִ��ͬ������ */
				{
					if(((raidStat[i] % 2) == 1) && (raidStat[i] >= 3) && (raidStat[i] < 25))
					{
						raidStat[i] = raidStat[i] + 2;
						continue;
					}
					/* ��raidͬ���Ľ���д����־ */
					time(&t);
					p = localtime(&t);
					fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
					fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
					strcpy(buffer1,"RAIDSTATUS|����");
					strncat(buffer1,temp + 4,2);  // �������е����
					strcat(buffer1,":����ͬ�� ����:");
					strncat(buffer1,strstr(buffer3,"resync = ") + 9,5);
					strcat(buffer1," ʣ��ʱ��:");
					strncat(buffer1,strstr(buffer3,"finish=") + 7,strstr(buffer3,"min") - strstr(buffer3,"finish=") - 7);
					strcat(buffer1,"��\n");
					fputs(buffer1,fp1);
					nLines ++;
					raidStat[i] = 3;
				}
				else  /* ���д�����������״̬ */
				{
					if(raidStat[i] == 0)
						continue;
					else
					{
						/* ��д����־ʱ�� */
						time(&t);
						p = localtime(&t);
						fprintf(fp1,"%d-%d-%d|",(1900 + p->tm_year),(1 + p->tm_mon),(p->tm_mday));
						fprintf(fp1,"%d:%02d:%02d|",p->tm_hour,p->tm_min,p->tm_sec);
						strcpy(buffer1,"RAIDSTATUS|����");
						strncat(buffer1,temp + 4,2);  // �������е����
						strcat(buffer1,":��������״̬\n");
						fputs(buffer1,fp1);
						nLines ++;
						raidStat[i] = 0;  /* �������м���raid�ĵ�ǰ����״̬ */
					}
				}
			}
		}
	}
	fclose(fp3);  /* �ر�/proc/mdstat�ļ� */
	/* ����ǰ����־�¼�д�ص�����־��[������������200������] */
	fp2 = fopen("log1","r");  /* ��ԭ������־�ļ� */
	while(fgets(buffer2,100,fp2) != NULL)
	{
		fputs(buffer2,fp1);
		nLines ++;
		if(nLines >= 200)
			break;
	}
	fclose(fp2);
	fclose(fp1);
	/* ɾ��ԭ������־�ļ� */
	system("rm -f log1");
}

// ��������ģ�飬������غ��޸�IP��
void Additional(RaidConfig& rc)
{
	FILE * fp1;
	char buffer1[100],cmdline1[100],cmdline2[100];
	chdir(workingDir);
	// �޸�����IP��ַ��DNS
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
			strncat(cmdline1,buffer1 + 7,strlen(buffer1) - 8);  /* ��ȡIP��ַ */
			strcat(cmdline1," netmask ");
			fgets(buffer1,100,fp1);  /* ��ȡ�������� */
			strncat(cmdline1,buffer1 + 8,strlen(buffer1) - 9);
			/* ��ȡĬ������ */
			fgets(buffer1,100,fp1);
			strncat(cmdline2,buffer1 + 8,strlen(buffer1) - 9);
		}
		system(cmdline1);  /* �޸�IP��ַ���������� */
		system(cmdline2);  /* �޸�Ĭ������ */
		fclose(fp1);
	}
	if(!access("resolv.conf",0))
	{
		system("chown root:root resolv.conf");
		system("mv -f resolv.conf /etc/resolv.conf");
	}
	// �رջ��������
	if(!access("reboot_event",0))
	{
		// ֹͣ�����������е�����
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			rc.singleRaids[i].stop();
		}
		system("rm -f reboot_event");
		system("reboot");
	}
	if(!access("halt_event",0))
	{
		// ֹͣ�����������е�����
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
	// ���������������
	DiskArray da;
	da.fillArray();
	// �����������ù������
	RaidConfig rc;
	// ��ʼ��raidStat����
	for(int i = 0;i < 32;i++)
		raidStat[i] = -1;
	if(access("raidConfig.xml",0))  // �����ǰ��xml�����ļ�������
	{
		for(int i = 0;i < da.diskNum;i++)
		{
			// �����̶�����Ϣ�������ù������rc��
			rc.addDisk(da.array[i]);
		}
		// ����xml�����ļ�raidConfig.xml
		rc.saveRcToXml("raidConfig.xml");
	}
	else  // ����Ѵ���xml�����ļ�
	{
		// ͨ��raidConfig.xml�ļ����rc����
		rc.buildRcFromXml("raidConfig.xml");
		// �����Ѿ����ڵ�����
		for(int i = 0;i < rc.singleRaidNum;i++)
		{
			rc.singleRaids[i].start();
		}
	}
	// ���������ļ�raidConfig.xml���������ù������rc�������
	// �������ѭ����
	while(1)
	{
		DiskMonitor(da,rc);
		RaidMonitor(rc);
		WriteLog();
		Additional(rc);
		sleep(10);
	}
}
