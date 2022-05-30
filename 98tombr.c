/* 98tombr - Shows PC-98 partition tables and writes an MBR equivalent for use on modern systems.
 * Copyright 2022 evv42.
 * See LICENSE file for copyright and license details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MBR_BOOTABLE        0x80
#define MBR_HCSECT_CYL      0xC0
#define MBR_HCSECT_SEC      0x3F
#define MBR_MAX_PARTS       4

#define PC98_MID_BOOTABLE   0x80
#define PC98_MID_MASK       0x7f
#define PC98_SID_ACTIVE     0x80
#define PC98_SID_MASK       0x7f
#define PC98_SYSS_FAT12     0x01
#define PC98_SYSS_PCUX      0x04
#define PC98_SYSS_N88       0x06
#define PC98_SYSS_FAT16A    0x11
#define PC98_SYSS_FAT16B    0x21
#define PC98_SYSS_NTFS      0x31
#define PC98_SYSS_BSD       0x44
#define PC98_SYSS_FAT32     0x61
#define PC98_SYSS_LINUX     0x62
#define PC98_MAX_PARTS      17

struct mbrpart {
	uint8_t flags;    /* bootstrap flags */
	uint8_t shd;      /* starting head */
	uint8_t shcsect;  /* starting cylinder(high bits)/sector */
	uint8_t scyl;     /* starting cylinder */
	uint8_t type;     /* partition type */
	uint8_t ehd;      /* end head */
	uint8_t ehcsect;  /* end sector */
	uint8_t ecyl;     /* end cylinder */
	uint32_t lbastart;/* absolute starting sector number */
	uint32_t lbasize; /* partition size in sectors */
};
typedef struct mbrpart mbrpart;

/*
 * Unlike MBR, the PC-98 stores its partition table at offset 0x200.
 * Each partition entry is 32 bytes long, one after the other, using the following stucture:
 */

struct pc98part {
	uint8_t  mid;  //aka boot
	uint8_t  sid;  //aka syss
	uint8_t  dum1; //empty
	uint8_t  dum2; //empty
	uint8_t  ipl_sct;  /*Initial Program Loader sector, usually the same as start of part*/
	uint8_t  ipl_head;
	uint16_t ipl_cyl;
	uint8_t  ssect;    /* starting sector */
	uint8_t  shd;      /* starting head */
	uint16_t scyl;     /* starting cylinder */
	uint8_t  esect;    /* end sector */
	uint8_t  ehd;      /* end head */
	uint16_t ecyl;     /* end cylinder */
	uint8_t  name[16];
};
typedef struct pc98part pc98part;

const char* pc98_type(uint8_t type){
	switch(type){
		case PC98_SYSS_PCUX: return "PC-UX (rare). Please upload it to archive.org.";
		case PC98_SYSS_N88: return "N88-BASIC";
		case PC98_SYSS_FAT12: return "FAT12";
		case PC98_SYSS_FAT16A: return "FAT16A";
		case PC98_SYSS_FAT16B: return "FAT16B";
		case PC98_SYSS_NTFS: return "IFS/HPFS/NTFS";
		case PC98_SYSS_BSD: return "386BSD";
		case PC98_SYSS_FAT32: return "FAT32";
		case PC98_SYSS_LINUX: return "Linux";
		default: return "?";
	}
	return "?";
}

void print_info_pc98(pc98part p, int i){
	printf("PC-98 Partition %d:\n",i);
	printf("mid: 0x%x %s\n", p.mid & PC98_MID_MASK, p.mid & PC98_MID_BOOTABLE ? "(bootable)" : "");
	printf("sid: 0x%x %s (%s)\n",p.sid & PC98_SID_MASK, p.sid & PC98_SID_ACTIVE ? "(active)" : "", pc98_type(p.sid & PC98_SID_MASK));
	printf("IPL   (C/H/S): %d/%d/%d\n",p.ipl_cyl,p.ipl_head,p.ipl_sct);
	printf("Start (C/H/S): %d/%d/%d\n",p.scyl,p.shd,p.ssect);
	printf("End   (C/H/S): %d/%d/%d\n",p.ecyl,p.ehd,p.esect);
	printf("Name: \"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\"\n",p.name[0],p.name[1],p.name[2],p.name[3],p.name[4],p.name[5],p.name[6]\
	,p.name[7],p.name[8],p.name[9],p.name[10],p.name[11],p.name[12],p.name[13],p.name[14],p.name[15]);
}

//The type list is reduced to what can technically be on a PC-98
//Since the MBR part is for modern OSes, we will avoid CHS madness here.
//That is what CONV98AT seems to do.
const char* mbr_type(uint8_t type){
	switch(type){
		case 0x07: return "IFS/HPFS/NTFS"; //OS/2 and NT
		case 0x0C: return "FAT32/LBA";     //Windows 98
		case 0x0E: return "FAT16B/LBA";    //DOS and Windows 95
		case 0x82: return "Linux swap";
		case 0x83: return "Linux native";
		default: return "?";
	}
	return "?";
}

//Based on reverse-enginering
uint8_t wildguess(uint8_t type){
	switch(type){
		case PC98_SYSS_PCUX:
		case PC98_SYSS_N88:
			printf("This partition format has no equivalent in MBR. Marked as free (0x00).\n");
			return 0x00;
		case PC98_SYSS_FAT12: return 0x01;
		case PC98_SYSS_FAT16A: return 0x04;
		case PC98_SYSS_FAT16B: return 0x0C;
		case PC98_SYSS_NTFS: return 0x07;
		case PC98_SYSS_BSD: return 0x00;
		case PC98_SYSS_FAT32: return 0x0C;
		case PC98_SYSS_LINUX: return 0x83;
		default: 
			printf("This partition has a unknown identifier. Marked as free (0x00).\n");
			return 0x00;
	}
	return 0x00;
}

void print_info_mbr(mbrpart p, int i){
	printf("MBR Partition %d:\n", i);
	printf("Flags        : 0x%x %s\n", p.flags, p.flags & MBR_BOOTABLE ? "(bootable/active)" : "");
	printf("Start (C/H/S): %d/%d/%d\n",p.scyl + ((MBR_HCSECT_CYL & p.shcsect)<<2),p.shd,p.shcsect & MBR_HCSECT_SEC);
	printf("End   (C/H/S): %d/%d/%d\n",p.ecyl + ((MBR_HCSECT_CYL & p.ehcsect)<<2),p.ehd,p.ehcsect & MBR_HCSECT_SEC);
	printf("Start   (LBA): %d\n",p.lbastart);
	printf("End     (LBA): %d (size %d/%dMB)\n",p.lbastart + p.lbasize,p.lbasize,(p.lbasize*512)/1048576);
	printf("Type         : 0x%x (%s)\n",p.type,mbr_type(p.type));
}

int print_help(char* name){
	printf("Shows PC-98 partition tables and writes an MBR equivalent for use on modern systems.\n");
	printf("Usage:\n");
	printf("%s -h : Show this\n\n",name);
	printf("%s -r or \n%s -read : Reads and displays the PC-98 partition table of the image/block device, and shows the corresponding MBR data already written.\n\n",name,name);
	printf("%s -s or \n%s -suggest: Suggest a MBR\n\n",name,name);
	printf("%s -w file or \n%s -write file or \n%s -wreck file : WRITES the suggested MBR to the image/block device. Make sure to select the correct file !\n\n",name,name,name);
	return 1;
}

pc98part* get_pc98_parts(FILE* drive){
	pc98part* parts = malloc(sizeof(pc98part)*PC98_MAX_PARTS);
	
	//skip to 0x200
	fseek(drive,0x200,SEEK_SET);
	
	int i=0;
	fread(&parts[i],sizeof(pc98part),1,drive);
	while(i < PC98_MAX_PARTS && parts[i].scyl != 0)fread(&parts[++i],sizeof(pc98part),1,drive);

	return parts;
}

mbrpart* get_mbr_suggestion(pc98part* parts){
	mbrpart* mparts = malloc(sizeof(mbrpart)*MBR_MAX_PARTS);
	memset(mparts,0,sizeof(mbrpart)*MBR_MAX_PARTS);
	
	for(int i = 0; i < MBR_MAX_PARTS && parts[i].scyl != 0; i++){
		mparts[i].flags = 0x00;
		mparts[i].shd = 0xFE;mparts[i].shcsect = 0xFF;mparts[i].scyl = 0xFF;
		mparts[i].ehd = 0xFE;mparts[i].ehcsect = 0xFF;mparts[i].ecyl = 0xFF;
		mparts[i].lbastart = parts[i].scyl * 136;
		mparts[i].lbasize = ((parts[i].ecyl - parts[i].scyl)+1) * 136;
		mparts[i].type = wildguess(parts[i].sid & PC98_SID_MASK);
	}
	
	return mparts;
}

int write_mbr(char* file){
	//open drive
	FILE* drive = fopen(file,"rb");
	
	pc98part* parts = get_pc98_parts(drive);
	int i;

	fclose(drive);
	
	printf("\nSuggested MBR:\n");

	mbrpart* mparts = get_mbr_suggestion(parts);
	for(i = 0; i < MBR_MAX_PARTS && mparts[i].type != 0; i++)print_info_mbr(mparts[i],i+1);
	
	free(parts);
	
	drive = fopen(file,"r+b");
	fseek(drive,0x1BE,SEEK_SET);
	for(i = 0; i < MBR_MAX_PARTS && mparts[i].type != 0; i++){
		long unsigned int r = fwrite(&mparts[i],sizeof(mbrpart),1,drive);
		if( r != 1 ){
			printf("Failed to write partition\n");
			free(mparts);
			return 2;
		}
	}
	fseek(drive,0x1FE,SEEK_SET);
	//write MBR signature
	const uint16_t signature = 0x55AA;
	long unsigned int r = fwrite(&signature,sizeof(uint16_t),1,drive);
	if( r != 1 ){
		printf("Failed to write MBR signature\n");
		free(mparts);
		return 3;
	}
	printf("Successfully written.\n");
	free(mparts);
	return 0;
}

int suggest_mbr(char* file){
	//open drive
	FILE* drive = fopen(file,"rb");
	
	pc98part* parts = get_pc98_parts(drive);
	int i;
	//for(i = 0; parts[i].scyl != 0; i++)print_info_pc98(parts[i],i+1);

	fclose(drive);
	
	printf("\nSuggested MBR:\n");

	mbrpart* mparts = get_mbr_suggestion(parts);
	for(i = 0; i < MBR_MAX_PARTS && mparts[i].type != 0; i++)print_info_mbr(mparts[i],i+1);
	
	free(parts);
	free(mparts);
	return 0;
}

int read_ptable(char* file){
	//open drive
	FILE* drive = fopen(file,"rb");
	
	pc98part* parts = get_pc98_parts(drive);
	int i;
	for(i = 0; parts[i].scyl != 0; i++)print_info_pc98(parts[i],i+1);
	free(parts);
	if(i == 0)printf("No PC-98 partition table.\n");
	
	printf("\nCorresponding MBR on disk:\n");
	//skip to 0x1BE
	fseek(drive,0x1BE,SEEK_SET);
	mbrpart mp;
	i=1;
	fread(&mp,sizeof(mbrpart),1,drive);
	while(mp.type != 0){
		print_info_mbr(mp,i);
		fread(&mp,sizeof(mbrpart),1,drive);
		i++;
	}
	
	fclose(drive);
	return 0;
}

int main(int argc, char** argv){
	//really basic switch handling here
	if(argc<3 || argv[1][0] != '-')return print_help(argv[0]);
	switch(argv[1][1]){
		case 'h': return print_help(argv[0]);
		case 'r': return read_ptable(argv[2]);
		case 's': return suggest_mbr(argv[2]);
		case 'w': return write_mbr(argv[2]);
	}
}
