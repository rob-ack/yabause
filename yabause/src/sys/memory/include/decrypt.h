#ifndef __DECRYPT_STV_H__
#define __DECRYPT_STV_H__

#ifdef __cplusplus
extern "C" {
#endif

u16 cryptoDecrypt(void);
void cryptoReset(void);
void cyptoSetKey(u32 privKey);
void cyptoSetLowAddr(u16 val);
void cyptoSetHighAddr(u16 val);
void cyptoSetSubkey(u16 subKey);

#ifdef __cplusplus
}
#endif

#endif
