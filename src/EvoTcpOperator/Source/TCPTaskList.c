#ifdef _WIN32
#define EvoTCPOperatorLibAPI __declspec(dllexport)
#else
#define EvoTCPOperatorLibAPI
#endif

#include "../Include/EvoTCPCommon.h"



TCPWriteTaskList* ConstructTCPWriteTaskList()
{
	TCPWriteTaskList* tmplist = (TCPWriteTaskList*)malloc(sizeof(TCPWriteTaskList));
	tmplist->count = 0;
	tmplist->size = 0;
	tmplist->task_head = tmplist->task_tail = NULL;
	return tmplist;
}

void ReleaseTCPTaskList(TCPWriteTaskList* list)
{
	while(list->task_head!=NULL)
	{
		list->task_tail = list->task_head->next;
		free(list->task_head->buf);
		free(list->task_head);
		list->task_head = list->task_tail;
	}
	free(list);
}


int InsertTCPWriteTaskToList(TCPWriteTaskList* list , const char* buf , u_int32_t buflength)
{
	int noMerge = 1;
	int bigData = 0;
	TCPNetworkTask* tmpnode;
	if(list->count == MAX_TCP_WRITE_TASKLIST_LENGTH)
		return ERR_TCP_WRITE_TASKLIST_FULL;

	if(list->task_head == NULL)
	{
		tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
		tmpnode->buf = NULL;
		tmpnode->buflength = 0;
		tmpnode->eleNum = 0;
		tmpnode->next = NULL;
		if(tmpnode == 0)
			return ERR_TCP_WRITE_TASKLIST_NOMEM;

		//ÿһ��С��MSG_ENCODE_BUF_LENGTH�����ݶ���Ҫ��һ������
		//�ϰ�֮��ÿһ���ӱ���ǰ����������
		if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
		{
			//����Ǵ����������Ҫ���������ȣ�ֻ��Ҫ�����ȷ�����Ϣͷ
			//���ⷢ��Header�����Ķ̱��ģ����ÿһ�����ݾ���ǰԤ��Header�Ŀռ�
			tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
			bigData = 1;
		}
		else
		{
			tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
		}
		if(tmpnode->buf == NULL)
		{
			free(tmpnode);
			return ERR_TCP_WRITE_TASKLIST_NOMEM;
		}
		if(bigData)
		{
			//���ȹ�����Ϣͷ
			EvoTCPMessageHeader msgHeader;
			msgHeader.type = EVO_TCP_MSG_DATA;
			msgHeader.flags = 0;
			msgHeader.octetsToNextHeader = buflength;
			msgHeader.extFlags = 1;
			//������Ϣͷ
			MapMsgHeader(tmpnode->buf , msgHeader);
			memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
			tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
		}
		else //С�����ݣ�ֻ��ҪԤ��Header�Ŀռ䣬������
		{
			if(tmpnode->buflength == 0) //��һ��С������
			{
					tmpnode->buflength  = SIZEOF_MSGHEADER; //Ԥ��Header�ռ�	
			}
			//����ֿ鳤��
			memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
			tmpnode->buflength += SIZEOF_DATASEGFLAG;
			memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
			tmpnode->buflength += buflength;
		}
		//��Ҫע�⣬������Ǵ����ݣ������һ��Ҫ����Header
		tmpnode->type = EVO_TCP_MSG_DATA;
		tmpnode->eleNum = 1; //��ʼ״̬��һ������
		tmpnode->next = NULL;
		list->task_head = list->task_tail = tmpnode;
		//printf("�����һ�����ݰ�%d\n" , tmpnode->buflength);
	}
	//�鿴��һ�������Ƿ�Ϊ�������ģ�����ǣ������ɾ��
	else if(list->task_head->next == NULL)
	{
		if(list->task_head->type == EVO_TCP_MSG_HEARTBEAT)
		{
			//ɾ��������
			free(list->task_head);
			list->task_head = list->task_tail = NULL;
			noMerge = 0; //����������൱�ڸ����ݱ������������ĺϲ�
			list->size-=SIZEOF_MSGHEADER;

			tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
			if(tmpnode == 0)
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			tmpnode->buflength = 0;
			tmpnode->buf = NULL;
			tmpnode->flags = 0;
			tmpnode->next = NULL;
			tmpnode->type = EVO_TCP_MSG_DATA;

			//��ʼ��������
			//ÿһ��С��MSG_ENCODE_BUF_LENGTH�����ݶ���Ҫ��һ������
			//�ϰ�֮��ÿһ���ӱ���ǰ����������
			if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
			{
				//����Ǵ����������Ҫ���������ȣ�ֻ��Ҫ�����ȷ�����Ϣͷ
				//���ⷢ��Header�����Ķ̱��ģ����ÿһ�����ݾ���ǰԤ��Header�Ŀռ�
				tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
				bigData = 1;
			}
			else
			{
				tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
			}
			if(tmpnode->buf == NULL)
			{
				free(tmpnode);
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			}

			if(bigData)
			{
				//���ȹ�����Ϣͷ
				EvoTCPMessageHeader msgHeader;
				msgHeader.type = EVO_TCP_MSG_DATA;
				msgHeader.flags = 0;
				msgHeader.octetsToNextHeader = buflength;
				msgHeader.extFlags = 1;
				//������Ϣͷ
				MapMsgHeader(tmpnode->buf , msgHeader);
				memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
				tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
			}
			else //С�����ݣ�ֻ��ҪԤ��Header�Ŀռ䣬������
			{
				if(tmpnode->buflength == 0) //��һ��С������
				{
					tmpnode->buflength  = SIZEOF_MSGHEADER; //Ԥ��Header�ռ�	
				}
				//����ֿ鳤��
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
				tmpnode->buflength += SIZEOF_DATASEGFLAG;
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
				tmpnode->buflength += buflength;
			}
			
			tmpnode->type = EVO_TCP_MSG_DATA;
			tmpnode->eleNum = 1; //��ʼ״̬��ֻ��һ������
			tmpnode->next = NULL;
			list->task_head = list->task_tail = tmpnode;
			
		}
		else //��һ��������������
		{
			//���Խ��кϰ�
			//�鿴��һ���뱾���ߴ�֮���Ƿ���ڱ��뻺��ߴ�
			//��ʱ����һ���ĳߴ����Ѿ�������Header�ĳߴ�
			if((list->task_head->buflength + buflength + SIZEOF_DATASEGFLAG) < MSG_ENCODE_BUF_LENGTH) //���Խ��кϰ�
			{
                u_int32_t tmpLength = list->task_head->buflength;
				memcpy((void*)(list->task_head->buf + tmpLength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
				memcpy((void*)(list->task_head->buf + tmpLength + SIZEOF_DATASEGFLAG) , (void*)buf , buflength);
				list->task_head->buflength = list->task_head->buflength + buflength + SIZEOF_DATASEGFLAG;
				list->task_head->eleNum++;
				noMerge = 0;
				//printf("����ϰ����ݰ�%d-%d\n" , list->task_head->eleNum , list->task_head->buflength);
			}
			else //���ϰ���������µĽڵ�
			{
				tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
				if(tmpnode == 0)
					return ERR_TCP_WRITE_TASKLIST_NOMEM;
				tmpnode->buflength = 0;
				tmpnode->buf = NULL;
				tmpnode->flags = 0;
				tmpnode->next = NULL;
				tmpnode->type = EVO_TCP_MSG_DATA;

				//ÿһ��С��MSG_ENCODE_BUF_LENGTH�����ݶ���Ҫ��һ������
				//�ϰ�֮��ÿһ���ӱ���ǰ����������
				if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
				{
					//����Ǵ����������Ҫ���������ȣ�ֻ��Ҫ�����ȷ�����Ϣͷ
					//���ⷢ��Header�����Ķ̱��ģ����ÿһ�����ݾ���ǰԤ��Header�Ŀռ�
					tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
					bigData = 1;
				}
				else
				{
					tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
				}
				if(tmpnode->buf == NULL)
				{
					free(tmpnode);
					return ERR_TCP_WRITE_TASKLIST_NOMEM;
				}
				if(bigData)
				{
					//���ȹ�����Ϣͷ
					EvoTCPMessageHeader msgHeader;
					msgHeader.type = EVO_TCP_MSG_DATA;
					msgHeader.flags = 0;
					msgHeader.octetsToNextHeader = buflength;
					msgHeader.extFlags = 1;
					//������Ϣͷ
					MapMsgHeader(tmpnode->buf , msgHeader);
					memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
					tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
				}
				else //С�����ݣ�ֻ��ҪԤ��Header�Ŀռ䣬������
				{
					if(tmpnode->buflength == 0) //��һ��С������
					{
						tmpnode->buflength  = SIZEOF_MSGHEADER; //Ԥ��Header�ռ�	
					}
					//����ֿ鳤��
					memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
					tmpnode->buflength += SIZEOF_DATASEGFLAG;
					memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
					tmpnode->buflength += buflength;
				}
				//��Ҫע�⣬������Ǵ����ݣ������һ��Ҫ����Header
				tmpnode->type = EVO_TCP_MSG_DATA;
				tmpnode->eleNum = 1; //��ʼ״̬��һ������
				tmpnode->next = NULL;

				list->task_tail->next = tmpnode;
				list->task_tail = tmpnode;
			}
		}//��һ��������������
	} //ֻ��һ������
	else //�Ѿ����ڶ������
	{
		//���Խ��кϰ�
		//�鿴��һ���뱾���ߴ�֮���Ƿ���ڱ��뻺��ߴ�
		if((list->task_tail->buflength + buflength + SIZEOF_DATASEGFLAG) < MSG_ENCODE_BUF_LENGTH) //���Խ��кϰ�
		{
            u_int32_t tmpLength = list->task_tail->buflength;
			memcpy((void*)(list->task_tail->buf + tmpLength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
			memcpy((void*)(list->task_tail->buf + tmpLength + SIZEOF_DATASEGFLAG) , (void*)buf , buflength);
			list->task_tail->buflength = list->task_tail->buflength + buflength + SIZEOF_DATASEGFLAG;
			list->task_tail->eleNum++;
			noMerge = 0;
			//printf("����ϰ����ݰ�%d-%d\n" , list->task_head->eleNum , list->task_tail->buflength);
		}
		else //���ϰ���������µĽڵ�
		{
			tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
			if(tmpnode == 0)
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			tmpnode->buflength = 0;
			tmpnode->buf = NULL;
			tmpnode->flags = 0;
			tmpnode->next = NULL;
			tmpnode->type = EVO_TCP_MSG_DATA;

			//ÿһ��С��MSG_ENCODE_BUF_LENGTH�����ݶ���Ҫ��һ������
			//�ϰ�֮��ÿһ���ӱ���ǰ����������
			if((buflength + SIZEOF_DATASEGFLAG + SIZEOF_MSGHEADER) > MSG_ENCODE_BUF_LENGTH)
			{
				//����Ǵ����������Ҫ���������ȣ�ֻ��Ҫ�����ȷ�����Ϣͷ
				//���ⷢ��Header�����Ķ̱��ģ����ÿһ�����ݾ���ǰԤ��Header�Ŀռ�
				tmpnode->buf = (char*)malloc(buflength+SIZEOF_MSGHEADER);
				bigData = 1;
			}
			else
			{
				tmpnode->buf = (char*)malloc(MSG_ENCODE_BUF_LENGTH); 
			}
			if(tmpnode->buf == NULL)
			{
				free(tmpnode);
				return ERR_TCP_WRITE_TASKLIST_NOMEM;
			}
			if(bigData)
			{
				//���ȹ�����Ϣͷ
				EvoTCPMessageHeader msgHeader;
				msgHeader.type = EVO_TCP_MSG_DATA;
				msgHeader.flags = 0;
				msgHeader.octetsToNextHeader = buflength;
				msgHeader.extFlags = 1;
				//������Ϣͷ
				MapMsgHeader(tmpnode->buf , msgHeader);
				memcpy((void*)(tmpnode->buf+SIZEOF_MSGHEADER) , (void*)buf , buflength);
				tmpnode->buflength = buflength+SIZEOF_MSGHEADER;
			}
			else //С�����ݣ�ֻ��ҪԤ��Header�Ŀռ䣬������
			{
				if(tmpnode->buflength == 0) //��һ��С������
				{
					tmpnode->buflength  = SIZEOF_MSGHEADER; //Ԥ��Header�ռ�	
				}
				//����ֿ鳤��
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)&buflength , SIZEOF_DATASEGFLAG);
				tmpnode->buflength += SIZEOF_DATASEGFLAG;
				memcpy((void*)(tmpnode->buf+tmpnode->buflength) , (void*)buf , buflength);
				tmpnode->buflength += buflength;
			}
			//��Ҫע�⣬������Ǵ����ݣ������һ��Ҫ����Header
			tmpnode->type = EVO_TCP_MSG_DATA;
			tmpnode->eleNum = 1; //��ʼ״̬��һ������
			tmpnode->next = NULL;

			list->task_tail->next = tmpnode;
			list->task_tail = tmpnode;
		}
	}


	list->count+=noMerge;

	if(bigData)
	{
		list->size+=buflength+SIZEOF_MSGHEADER;
		//printf("��������� : %d\n" , list->task_tail->buflength);
	}
	else if(noMerge)
	{
		EvoTCPMessageHeader msgHeader;
		tmpnode = list->task_tail;
		//����Header
		//���ȹ�����Ϣͷ
		
		msgHeader.type = EVO_TCP_MSG_DATA;
		msgHeader.flags = 1; //����һ�ֿ����ԣ�ֻ��һ�������ݣ�����ΪС�����ݣ�û�кϰ�
		msgHeader.octetsToNextHeader = buflength+SIZEOF_DATASEGFLAG;
		msgHeader.extFlags = tmpnode->eleNum;
		//������Ϣͷ
		MapMsgHeader(tmpnode->buf , msgHeader);
		list->size+=buflength+SIZEOF_MSGHEADER+SIZEOF_DATASEGFLAG;
		//printf("����С���ݣ����ϰ� : %d\n" ,list->task_tail->buflength );
	}
	else
	{
		EvoTCPMessageHeader msgHeader;
		tmpnode = list->task_tail;
		//����Header
		//���ȹ�����Ϣͷ
		
		msgHeader.type = EVO_TCP_MSG_DATA;
		msgHeader.flags = 1;
		msgHeader.octetsToNextHeader = list->task_tail->buflength - SIZEOF_MSGHEADER;
		msgHeader.extFlags = tmpnode->eleNum;
		//������Ϣͷ
		MapMsgHeader(list->task_tail->buf , msgHeader);
		list->size+=buflength+SIZEOF_DATASEGFLAG;
		//printf("����С���ݣ��ϰ� : %d\n" ,list->task_tail->buflength );
	}

	return list->count;
}

int InsertTCPHeartbeatToList(TCPWriteTaskList* list )
{
	TCPNetworkTask* tmpnode;
	if(list->count == MAX_TCP_WRITE_TASKLIST_LENGTH)
		return ERR_TCP_WRITE_TASKLIST_FULL;
	tmpnode = (TCPNetworkTask*)malloc(sizeof(TCPNetworkTask));
	if(tmpnode == 0)
		return ERR_TCP_WRITE_TASKLIST_NOMEM;
	tmpnode->buf = NULL;
	tmpnode->buflength = 0;
	tmpnode->type = EVO_TCP_MSG_HEARTBEAT;
	tmpnode->next = NULL;

	if(list->task_head == NULL)
	{
		list->task_head = list->task_tail = tmpnode;
		list->count+=1;
		list->size+=SIZEOF_MSGHEADER;
		//printf("׼����������\n");
	}
	
	return list->count;
}

void  SwitchTaskList(TCPWriteTaskList* tgtList , TCPWriteTaskList* srcList)
{

	tgtList->count = srcList->count;
	tgtList->size = srcList->size;
	tgtList->task_head = srcList->task_head;
	tgtList->task_tail = srcList->task_tail;

	srcList->count = 0;
	srcList->size = 0;
	srcList->task_head = srcList->task_tail = NULL;
}