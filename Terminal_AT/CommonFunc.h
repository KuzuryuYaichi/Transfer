#ifndef _COMMONFUNC_TERMINAL_H_
#define _COMMONFUNC_TERMINAL_H_

/***********************外部调用的接口函数*********************/
int StrToHex(char *ch,__u32 len,char *hex);
char valueToHexCh(const int value);
__u64 Hex_Dec(char *HexData,int HexData_len);
__u64 Hex_to_Dec(__u8 *s);
void Dec_to_Hex(__u64 Dec,__u8 *Hex);
char* ID_to_Token(__u8 *id,__u8 id_len,char *token);
int Dec2Hex(__u64 Dec, __u8 *Hex);
__u8 Hex2Dec(__u8 HexData);
char* num2str(char *at_send,int len);
__u8 findKneeNum(__u64 knee_SN);
/***********************写入记录信息的接口函数*********************/
void site_id_reset(void* value,__u8 type);
int ResetRecord();
int ConnectRecord();
int DisConnectRecord();
void WriteFlags(__u8 isPCRD,__u8 isCOUNT,__u8 flag);
int ReadFlags();

#endif
