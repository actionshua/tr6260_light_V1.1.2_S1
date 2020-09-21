#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "LzmaLib.h"


#define MIN(x,y) (((x)<(y)) ? (x) : (y))

static void split(off_t *I,off_t *V,off_t start,off_t len,off_t h)
{
	off_t i,j,k,x,tmp,jj,kk;

	if(len<16) {
		for(k=start;k<start+len;k+=j) {
			j=1;x=V[I[k]+h];
			for(i=1;k+i<start+len;i++) {
				if(V[I[k+i]+h]<x) {
					x=V[I[k+i]+h];
					j=0;
				};
				if(V[I[k+i]+h]==x) {
					tmp=I[k+j];I[k+j]=I[k+i];I[k+i]=tmp;
					j++;
				};
			};
			for(i=0;i<j;i++) V[I[k+i]]=k+j-1;
			if(j==1) I[k]=-1;
		};
		return;
	};

	x=V[I[start+len/2]+h];
	jj=0;kk=0;
	for(i=start;i<start+len;i++) {
		if(V[I[i]+h]<x) jj++;
		if(V[I[i]+h]==x) kk++;
	};
	jj+=start;kk+=jj;

	i=start;j=0;k=0;
	while(i<jj) {
		if(V[I[i]+h]<x) {
			i++;
		} else if(V[I[i]+h]==x) {
			tmp=I[i];I[i]=I[jj+j];I[jj+j]=tmp;
			j++;
		} else {
			tmp=I[i];I[i]=I[kk+k];I[kk+k]=tmp;
			k++;
		};
	};

	while(jj+j<kk) {
		if(V[I[jj+j]+h]==x) {
			j++;
		} else {
			tmp=I[jj+j];I[jj+j]=I[kk+k];I[kk+k]=tmp;
			k++;
		};
	};

	if(jj>start) split(I,V,start,jj-start,h);

	for(i=0;i<kk-jj;i++) V[I[jj+i]]=kk-1;
	if(jj==kk-1) I[jj]=-1;

	if(start+len>kk) split(I,V,kk,start+len-kk,h);
}

static void qsufsort(off_t *I,off_t *V,u_char *old,off_t oldsize)
{
	off_t buckets[256];
	off_t i,h,len;

	for(i=0;i<256;i++) buckets[i]=0;
	for(i=0;i<oldsize;i++) buckets[old[i]]++;
	for(i=1;i<256;i++) buckets[i]+=buckets[i-1];
	for(i=255;i>0;i--) buckets[i]=buckets[i-1];
	buckets[0]=0;

	for(i=0;i<oldsize;i++) I[++buckets[old[i]]]=i;
	I[0]=oldsize;
	for(i=0;i<oldsize;i++) V[i]=buckets[old[i]];
	V[oldsize]=0;
	for(i=1;i<256;i++) if(buckets[i]==buckets[i-1]+1) I[buckets[i]]=-1;
	I[0]=-1;

	for(h=1;I[0]!=-(oldsize+1);h+=h) {
		len=0;
		for(i=0;i<oldsize+1;) {
			if(I[i]<0) {
				len-=I[i];
				i-=I[i];
			} else {
				if(len) I[i-len]=-len;
				len=V[I[i]]+1-i;
				split(I,V,i,len,h);
				i+=len;
				len=0;
			};
		};
		if(len) I[i-len]=-len;
	};

	for(i=0;i<oldsize+1;i++) I[V[i]]=i;
}

static off_t matchlen(u_char *old,off_t oldsize,u_char *new,off_t newsize)
{
	off_t i;

	for(i=0;(i<oldsize)&&(i<newsize);i++)
		if(old[i]!=new[i]) break;

	return i;
}

static off_t search(off_t *I,u_char *old,off_t oldsize,
		u_char *new,off_t newsize,off_t st,off_t en,off_t *pos)
{
	off_t x,y;

	if(en-st<2) {
		x=matchlen(old+I[st],oldsize-I[st],new,newsize);
		y=matchlen(old+I[en],oldsize-I[en],new,newsize);

		if(x>y) {
			*pos=I[st];
			return x;
		} else {
			*pos=I[en];
			return y;
		}
	};

	x=st+(en-st)/2;
	if(memcmp(old+I[x],new,MIN(oldsize-I[x],newsize))<0) {
		return search(I,old,oldsize,new,newsize,x,en,pos);
	} else {
		return search(I,old,oldsize,new,newsize,st,x,pos);
	};
}

static void offtout(off_t x,u_char *buf)
{
	off_t y;

	if(x<0) y=-x; else y=x;

		buf[0]=y%256;y-=buf[0];
	y=y/256;buf[1]=y%256;y-=buf[1];
	y=y/256;buf[2]=y%256;y-=buf[2];
	y=y/256;buf[3]=y%256;y-=buf[3];
	y=y/256;buf[4]=y%256;y-=buf[4];
	y=y/256;buf[5]=y%256;y-=buf[5];
	y=y/256;buf[6]=y%256;y-=buf[6];
	y=y/256;buf[7]=y%256;

	if(x<0) buf[7]|=0x80;
}

unsigned char prop[5] = { 0 };
size_t sizeProp = 5;

unsigned char * SwGetLzmaProp()
{
    return prop;
}

/*
 * dict used to find the same strings in the area to compress the data
 */
int SwLzmaCompress(unsigned char *src, size_t srcLen, char *dst, size_t *dstLen)
{
    if (LzmaCompress(dst, dstLen, src, srcLen, prop, &sizeProp, 5, (1 << 13)/* 8k dict */, 0, 0, 0, 32, 2) !=  0) {
        printf("LzmaCompress error\n");
        return -1;
    }

    return 0;
}

int SwBsdiff(unsigned char *oldBuff, unsigned int oldLen, unsigned char *newBuff, unsigned int newLen,
    unsigned char *outBuff, unsigned int *outLen)
{
    off_t *I,*V;
    off_t scan,pos,len;
    off_t lastscan,lastpos,lastoffset;
    off_t oldscore,scsc;
    off_t s,Sf,lenf,Sb,lenb;
    off_t overlap,Ss,lens;
    off_t i;
    off_t dblen,eblen, cblen;
    u_char *db,*eb,*cb, *allb;
    u_char buf[8];
    u_char header[32];
    size_t tmpLen;

    if (((I=malloc((oldLen+1)*sizeof(off_t)))==NULL) || ((V=malloc((oldLen+1)*sizeof(off_t)))==NULL)) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        return -1;
    }
    qsufsort(I, V, oldBuff, oldLen);
    free(V);

    if(((db=malloc(newLen+1))==NULL) || ((eb=malloc(newLen+1))==NULL) || ((cb=malloc(newLen+1))==NULL) || ((allb=malloc(newLen*2))==NULL)) {
        printf("%s[%d] No Mem Left\n", __FUNCTION__, __LINE__);
        return -1;
    }
    dblen=0;
    eblen=0;
    cblen = 0;

	scan=0;len=0;
	lastscan=0;lastpos=0;lastoffset=0;
	while(scan<newLen) {
		oldscore=0;

		for(scsc=scan+=len;scan<newLen;scan++) {
			len=search(I,oldBuff,oldLen,newBuff+scan,newLen-scan,
					0,oldLen,&pos);

			for(;scsc<scan+len;scsc++)
			if((scsc+lastoffset<oldLen) &&
				(oldBuff[scsc+lastoffset] == newBuff[scsc]))
				oldscore++;

			if(((len==oldscore) && (len!=0)) || 
				(len>oldscore+8)) break;

			if((scan+lastoffset<oldLen) &&
				(oldBuff[scan+lastoffset] == newBuff[scan]))
				oldscore--;
		};

		if((len!=oldscore) || (scan==newLen)) {
			s=0;Sf=0;lenf=0;
			for(i=0;(lastscan+i<scan)&&(lastpos+i<oldLen);) {
				if(oldBuff[lastpos+i]==newBuff[lastscan+i]) s++;
				i++;
				if(s*2-i>Sf*2-lenf) { Sf=s; lenf=i; };
			};

			lenb=0;
			if(scan<newLen) {
				s=0;Sb=0;
				for(i=1;(scan>=lastscan+i)&&(pos>=i);i++) {
					if(oldBuff[pos-i]==newBuff[scan-i]) s++;
					if(s*2-i>Sb*2-lenb) { Sb=s; lenb=i; };
				};
			};

			if(lastscan+lenf>scan-lenb) {
				overlap=(lastscan+lenf)-(scan-lenb);
				s=0;Ss=0;lens=0;
				for(i=0;i<overlap;i++) {
					if(newBuff[lastscan+lenf-overlap+i]==
					   oldBuff[lastpos+lenf-overlap+i]) s++;
					if(newBuff[scan-lenb+i]==
					   oldBuff[pos-lenb+i]) s--;
					if(s>Ss) { Ss=s; lens=i+1; };
				};

				lenf+=lens-overlap;
				lenb-=lens;
			};

			for(i=0;i<lenf;i++)
				db[dblen+i]=newBuff[lastscan+i]-oldBuff[lastpos+i];
			for(i=0;i<(scan-lenb)-(lastscan+lenf);i++)
				eb[eblen+i]=newBuff[lastscan+lenf+i];

			dblen+=lenf;
			eblen+=(scan-lenb)-(lastscan+lenf);

			offtout(lenf,buf);
            memcpy(cb + cblen, buf, 8);
            cblen += 8;
			offtout((scan-lenb)-(lastscan+lenf),buf);
            memcpy(cb + cblen, buf, 8);
            cblen += 8;

			offtout((pos-lenb)-(lastpos+lenf),buf);
			memcpy(cb + cblen, buf, 8);
            cblen += 8;

			lastscan=scan-lenb;
			lastpos=pos-lenb;
			lastoffset=pos-scan;
		};
	};

	/* Header is
		0	8	 "BSDIFF40"
		8	8	length of bzip2ed ctrl block
		16	8	length of bzip2ed diff block
		24	8	length of new file */
	/* File is
		0	32	Header
		32	??	Bzip2ed ctrl block
		??	??	Bzip2ed diff block
		??	??	Bzip2ed extra block */
	memcpy(header,"BSDIFF40",8);
	offtout(cblen, header + 8);
	offtout(dblen, header + 16);
	offtout(newLen, header + 24);

    //printf("%d-%d\n", (cblen + dblen + eblen + 32), 2*newLen);
    if ((cblen + dblen + eblen + 32) > 2 * newLen) {
        printf("outBuff not enough\n");
        return -1;
    }

    memcpy(allb, cb, cblen);
    memcpy(allb + cblen, db, dblen);
    memcpy(allb + cblen + dblen, eb, eblen);

    //BsdiffLzmaCompress(header, tt, eblen+ctrlBuffLen+dblen, argv[3], "allsrc");
    tmpLen = 2*newLen;
    SwLzmaCompress(allb, cblen+eblen+dblen, outBuff+32, &tmpLen);
    memcpy(outBuff, header, sizeof(header));
    //memcpy(outBuff + 32, allb, cblen+eblen+dblen);
    //tmpLen = cblen+eblen+dblen;
    *outLen = tmpLen + 32;

    /* Free the memory we used */
    free(db);
    free(eb);
    free(cb);
    free(allb);
    free(I);

    return 0;
}
