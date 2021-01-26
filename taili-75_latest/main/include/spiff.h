#ifndef __spiffs_H__
#define __spiffs_H__



uint8_t sf_WriteBuffer(uint8_t *_pBuf, uint32_t _uiWriteAddr, uint16_t _usWriteSize);
typedef struct
{
    unsigned int ChipID;    /* оƬID */
    char ChipName[16];      /* оƬ�ͺ��ַ�������Ҫ������ʾ */
    unsigned int TotalSize; /* ������ */
    unsigned int PageSize;  /* ҳ���С */
} SFLASH_T;

#define info_page 1280    //�洢ͼƬ���� ���Ƶ���Ϣ
#define info_page_backup 1281    //�洢ͼƬ���� ���Ƶ���Ϣ���ڱ���
#define picture_cap 15    //  ͼƬ��ռ�����ռ�40 ��СΪ40*4096
#define low_network_wifi_picture_page 1282		//�洢�͵�ѹͼƬ 1282
//#define picture_page 1283 //��1283������ʼ�洢��ÿ��ͼƬռpicture_cap��С

//#define info_pic_name_for_err	info_page*4096+500
//#define info_pic_name	info_page*4096+600 //�����￪ʼ�洢ͼƬ���� ÿ��ͼƬ���ֳ���Ϊ20B
//#define composite_picture_page 1342  //�洢�ϳ�ͼƬ 1342~1350


#endif
