#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _REDHAT_V9
#include <errno.h>
#endif

#include "title.h"
#include "char.h"
#include "char_base.h"
#include "item.h"
#include "skill.h"
#include "buf.h"
#include "util.h"
#include "configfile.h"


/*====================�ι�ɽ====================*/
static int TITLE_IntCheck( int charaindex,int elem, int *data, int flg);
static int TITLE_WorkIntCheck( int charaindex,int elem, int *data, int flg);
static int TITLE_ItemCheck( int charaindex,int elem, int *data, int flg);
static int TITLE_ItemEquipCheck( int charaindex,int elem, int *data, int flg);
static int TITLE_SkillCheck( int charaindex,int elem, int *data, int flg);
static int TITLE_SexCheck( int charaindex,int elem, int *data, int flg);

typedef enum
{
	TITLE_FUNCTYPENONE,     /*  �ؿ��������ʤ�  */
	TITLE_FUNCTYPEUSERFUNC, /* definefunction ��Ȥä�̾������
							 * �ؿ��ΰ�����
							 *  int     ����饤��ǥå���
							 *  buf     ̾���ؤΥХåե�
							 *  buflen  ���ΥХåե���Ĺ��
							 */
	TITLE_USEFUNCTYPENUM
}TITLE_USEFUNCTYPE;

typedef struct tagTITLE_Table
{
	int                 index;      /* ��ǡ����ȸߴ����뤿�ᡣ
									 * �����ֹ�Ǥ�ä�addtitle�Ȥ���� 
									 */
	char               name[32];
	TITLE_USEFUNCTYPE   functype;
	void                (*definefunction)(int,char* buf,int buflen);
}TITLE_Table;

/* �Ȥ���ѥ�᡼��
 * STR,TGH,MAXMP��ATK,DEF
 * LEVEL,CLASS,SKILL,ITEM,FIREREG,ICEREG,THUNDERREG
 * KANJILV,TALKCNT,WALKCNT,DEADCNT,LOGINCNT,BASEIMAGENUMBER
 * GOLD
*/
typedef struct tagTITLE_Compare {
	char    compare[8];;
} TITLE_COMPARE;

TITLE_COMPARE TITLE_compare[] = { {"<="}, {">="},{"<>"}, {">"},{"<"},{"="}};

typedef struct tagTITLE_PARAM
{
	int     element;
	int     (*checkfunc)( int charaindex,int elem, int *data, int flg );
	char    *paramname;
}TITLE_PARAM;

TITLE_PARAM TITLE_param[] = 
{
	/**** �������� ****/
	{ -1,                     TITLE_ItemCheck,     "ITEM"         },
	{ -1,                     TITLE_ItemEquipCheck,"EQUIPITEM"    },
	{ -1,                     NULL,                "EQUIPEVENT"   },
	/**** �����ޤǤ��ѹ����ʤ��Ǥ��������� ****/
	{ -1,                     TITLE_SkillCheck,    "SKILL"        },
	{ CHAR_STR,               TITLE_IntCheck,      "STR"           },
	{ CHAR_TOUGH,             TITLE_IntCheck,      "TGH"           },
	{ CHAR_MAXMP,             TITLE_IntCheck,      "MAXMP"         },
	{ CHAR_WORKFIXSTR,        TITLE_WorkIntCheck,  "ATK"           },
	{ CHAR_WORKFIXTOUGH,      TITLE_WorkIntCheck,  "DEF"           },
	{ CHAR_LV,                TITLE_IntCheck,      "LEVEL"         },
	{ CHAR_TALKCOUNT,         TITLE_IntCheck,      "TALKCNT"       },
	{ CHAR_WALKCOUNT,         TITLE_IntCheck,      "WALKCNT"       },
	{ CHAR_DEADCOUNT,         TITLE_IntCheck,      "DEADCNT"       },
	{ CHAR_LOGINCOUNT,        TITLE_IntCheck,      "LOGINCNT"      },
	{ CHAR_BASEBASEIMAGENUMBER,TITLE_IntCheck,      "BASEIMAGENUMBER" },
	{ CHAR_GOLD,              TITLE_IntCheck,      "GOLD"          },
	{ -1,                     TITLE_SexCheck,      "SEX"           },
};

#define     TITLE_PARAMSIZE     20
typedef struct tagTITLE_configTable
{
	int     paramindex[20];         /* TITLE_param�ؤ�ź�� */
	int     param[20][TITLE_PARAMSIZE];              /* ɾ���� */
	int     compareflg[20];         /* ><=���ɤ����뤫 */
	int     title;
	BOOL    equipcheckflg;          /* 
									 *  �����ƥ��ư���٥�ȤǤϤ��Υե饰��Ω�äƤ���
									 *  ��Τ����������ʤ���
									 */
}TITLE_CONFIGTABLE;

typedef struct tagTITLE_configbuf
{
	int     title;
	int     flg;
}TITLE_CONFIGBUF;

static TITLE_CONFIGTABLE    *TITLE_ConfigTable;
static TITLE_Table          *TITLE_table;
static TITLE_CONFIGBUF      *TITLE_configbuf;
static int                  TITLE_titlenum;
static int                  TITLE_titlecfgnum;

/*------------------------------------------------------------
 * index�ֹ椫��TITLE_table��ź��������
 ------------------------------------------------------------*/
int TITLE_getTitleIndex( int index)
{
	int i;
	if( index < 0 ) return -1;
	for( i = 0; i < TITLE_titlenum; i ++ ) {
		if( TITLE_table[i].index == index ) {
			return( i);
		}
	}
	return -1;
}

/*  �Хåե��Υ�����    */
#define TITLESTRINGBUFSIZ   256
/*  ���饤����Ȥ˸����륹����ǡ�����ʸ����ΥХåե�    */
static char    TITLE_statusStringBuffer[TITLESTRINGBUFSIZ];
/*------------------------------------------------------------
 * ���饤����Ȥ˸�����ι��ʸ�������
 * ����
 *  title       Title*      ������
 *  charaindex  int         ���ξι����äƤ��륭���Υ���ǥå���
 * �֤���
 *  char*
 ------------------------------------------------------------*/
char* TITLE_makeTitleStatusString( int charaindex,int havetitleindex )
{
	int     attach;
	int     index;
	/*  �ؿ�ɽ�ؤΥ���ǥå�������ǡ������������  */
	index = CHAR_getCharHaveTitle( charaindex,havetitleindex );
#if 0
	if( TITLE_CHECKTABLEINDEX( index ) == FALSE ){
		TITLE_statusStringBuffer[0] = '\0';
		return TITLE_statusStringBuffer;
	}
#endif
	attach = TITLE_getTitleIndex( index);
	if( attach == -1 ) {
		TITLE_statusStringBuffer[0] = '\0';
		return TITLE_statusStringBuffer;
	}
	switch( TITLE_table[attach].functype ){
	case TITLE_FUNCTYPENONE:
		snprintf( TITLE_statusStringBuffer,
				  sizeof(TITLE_statusStringBuffer ),"%s" ,
				  TITLE_table[attach].name );
		break;
	
	case TITLE_FUNCTYPEUSERFUNC:
	{
		char    string[256]={""};
		void    (*function)(int,char* buf,int buflen);
		function = TITLE_table[attach].definefunction;
		if( function )
			function( charaindex,string,sizeof(string) );

		strcpysafe( TITLE_statusStringBuffer,
					sizeof(TITLE_statusStringBuffer ),string );
	}
	break;
	default:
		TITLE_statusStringBuffer[0] = '\0';
		return TITLE_statusStringBuffer;
		break;
	}
	return TITLE_statusStringBuffer;
}

/*------------------------------------------------------------
 * �ʤ��ι��ʸ����ǡ������֤�
 * ����
 *  �ʤ�
 * �֤���
 *  char*
 ------------------------------------------------------------*/
char* TITLE_makeSkillFalseString( void )
{
	TITLE_statusStringBuffer[0]= '\0';
	return TITLE_statusStringBuffer;
}


/*------------------------------------------------------------
 * ���ꤵ�줿�ֹ�ξι���ɲä��롣�Ť��ʤäƤ����顢�ɲä��ʤ�
 * ����
 *  charaindex      int     ����饤��ǥå���
 *  titleindex      int     �ι楤��ǥå���
 * �֤���
 *  �ɲä���index       
 *  �ɲä��ʤ��ä�      FALSE(0)
 ------------------------------------------------------------*/
BOOL TITLE_addtitle( int charaindex, int titleindex )
{
	int i;
	int firstfindempty=-1;

	if( CHAR_CHECKINDEX(charaindex)  == FALSE )return FALSE;
	/*if( TITLE_CHECKTABLEINDEX(titleindex) == FALSE )return FALSE;*/
	if( TITLE_getTitleIndex( titleindex) == -1 ) return FALSE;

	for( i=0 ; i < CHAR_TITLEMAXHAVE ; i++ ){
		if( CHAR_getCharHaveTitle( charaindex,i ) == titleindex )
			/*  ���Ǥ˻��äƤ��뤫�鲿�⤷�ʤ�  */
			return FALSE;
		if( firstfindempty == -1
			&& CHAR_getCharHaveTitle(charaindex,i) == -1 ){
			firstfindempty = i;
		}
	}
	return CHAR_setCharHaveTitle( charaindex,firstfindempty,titleindex );
}


/*------------------------------------------------------------
 * ���ꤵ�줿�ֹ�ξι椬���ä��������롣�ʤ��ä��鲿�⤷�ʤ���
 * ʣ���Ĥ��ä��������ä���
 * ����
 *  charaindex      int     ����饤��ǥå���
 *  titleindex      int     �ι楤��ǥå���
 * �֤���
 *  �������            TRUE(1)
 *  ������ʤ��ä�      FALSE(0)
 ------------------------------------------------------------*/
BOOL TITLE_deltitle( int charaindex, int titleindex )
{
	int i;
	BOOL    del=FALSE;
	int     index;
	if( CHAR_CHECKINDEX(charaindex)  == FALSE )return FALSE;
	/*if( TITLE_CHECKTABLEINDEX(titleindex) == FALSE )return FALSE;*/
	index = TITLE_getTitleIndex( titleindex);
	if(  index == -1 ) return FALSE;

	for( i=0 ; i < CHAR_TITLEMAXHAVE ; i++ )
		if( CHAR_getCharHaveTitle( charaindex,i ) == titleindex ){
			/*  ��ʬ���ȤäƤ�����Ĥʤ�С������ʤ��ˤ���    */
			if( CHAR_getInt(charaindex, CHAR_INDEXOFEQTITLE) == i ){
				CHAR_setInt(charaindex, CHAR_INDEXOFEQTITLE, -1 );
			}
			/*  ���äƤ��뤫��ä�  */
			CHAR_setCharHaveTitle( charaindex,i,-1);
			
			del = TRUE;
		}

	return del;
}
/*------------------------------------------------------------
 * �ι�ν�����򤹤롣
 * ����
 *  filename        char*       ����ե�����̾
 * �֤���
 *  ����    TRUE(1)
 *  ����    FALSE(0)
 *------------------------------------------------------------*/
BOOL TITLE_initTitleName( char* filename )
{
	FILE*   f;
	char    line[256];
	int     linenum=0;
	int     title_readlen=0;

	f = fopen(filename,"r");
	if( f == NULL ){
		errorprint;
		return FALSE;
	}

	TITLE_titlenum=0;

	/*  �ޤ�ͭ���ʹԤ����Ԥ��뤫�ɤ���Ĵ�٤�    */
	while( fgets( line, sizeof( line ), f ) ){
		linenum ++;
		if( line[0] == '#' )continue;        /* comment */
		if( line[0] == '\n' )continue;       /* none    */
		chomp( line );

		TITLE_titlenum++;
	}

	if( fseek( f, 0, SEEK_SET ) == -1 ){
		fprint( "Seek Error\n" );
		fclose(f);
		return FALSE;
	}

	TITLE_table = allocateMemory( sizeof(struct tagTITLE_Table)
								   * TITLE_titlenum );
	if( TITLE_table == NULL ){
		fprint( "Can't allocate Memory %d\n" ,
				sizeof(TITLE_table)*TITLE_titlenum);
		fclose( f );
		return FALSE;
	}
	/* ����� */
{
	int     i;
	for( i = 0; i < TITLE_titlenum; i ++ ) {
		TITLE_table[i].index = -1;
		TITLE_table[i].name[0] = '\0';
		TITLE_table[i].functype = TITLE_FUNCTYPENONE;
		TITLE_table[i].definefunction = NULL;
	}
	
}

	/*  �ޤ��ɤ�ľ��    */
	linenum = 0;
	while( fgets( line, sizeof( line ), f ) ){
		linenum ++;
		if( line[0] == '#' )continue;        /* comment */
		if( line[0] == '\n' )continue;       /* none    */
		chomp( line );

		/*  �Ԥ���������    */
		/*  �ޤ� tab �� " " ���֤�������    */
		replaceString( line, '\t' , ' ' );
		/* ��Ƭ�Υ��ڡ������롣*/
{
		int     i;
		char    buf[256];
		for( i = 0; i < strlen( line); i ++) {
			if( line[i] != ' ' ) {
				break;
			}
			strcpy( buf, &line[i]);
		}
		if( i != 0 ) {
			strcpy( line, buf);
		}
}
{
		char    token[256];
		int     ret;

		/*  �ҤȤĤ�Υȡ�����򸫤�    */
		ret = getStringFromIndexWithDelim( line,",",1,token,
										   sizeof(token));
		if( ret==FALSE ){
			fprint("Syntax Error file:%s line:%d\n",filename,linenum);
			continue;
		}
		TITLE_table[title_readlen].index = atoi(token);

		/*  2�Ĥ�Υȡ�����򸫤�    */
		ret = getStringFromIndexWithDelim( line,",",2,token,
										   sizeof(token));
		if( ret==FALSE ){
			fprint("Syntax Error file:%s line:%d\n",filename,linenum);
			continue;
		}
		if( strlen( token) > sizeof( TITLE_table[title_readlen].name)-1) {
			fprint("Warning! TitleName Length Over file:%s line:%d\n",
					filename, linenum);
		}
		strcpysafe( TITLE_table[title_readlen].name, 
					sizeof( TITLE_table[title_readlen].name),
					token);

		title_readlen ++;
}
	}
	fclose(f);

	TITLE_titlenum = title_readlen;

	print( "Valid Title Num is %d...", TITLE_titlenum );

#ifdef DEBUG

	{
		int i;
		for( i=0; i <TITLE_titlenum ; i++ )
			print( "Title index[%d] name[%s] \n",
				   TITLE_table[i].index,
				   TITLE_table[i].name);
	}
#endif
	return TRUE;
}
/*------------------------------------------------------------
 * �ι�κƽ�����򤹤롣
 * ����
 *  filename        char*       ����ե�����̾
 * �֤���
 *  ����    TRUE(1)
 *  ����    FALSE(0)
 *------------------------------------------------------------*/
BOOL TITLE_reinitTitleName( void)
{
	freeMemory( TITLE_table);
	return(TITLE_initTitleName( getTitleNamefile()));
}
/*------------------------------------------------------------
 * �ι�����ν�����򤹤롣
 * ����
 *  filename        char*       ����ե�����̾
 * �֤���
 *  ����    TRUE(1)
 *  ����    FALSE(0)
 *------------------------------------------------------------*/
static void TITLE_initTitleData( int array)
{
	int j,k;
	for( j = 0; j < arraysizeof( TITLE_ConfigTable[array].paramindex); j ++) {
		for( k = 0; k < TITLE_PARAMSIZE; k++) {
			TITLE_ConfigTable[array].param[j][k] = -1;
		}
		TITLE_ConfigTable[array].paramindex[j] = -1;
		TITLE_ConfigTable[array].compareflg[j] = -1;
	}
	TITLE_ConfigTable[array].title = -1;
	TITLE_ConfigTable[array].equipcheckflg = FALSE;
	
}
/*------------------------------------------------------------
 * ʣ���������ޤȤ�ƣ��Ԥˤ��Ƥ�롣�ɤ�Τ⤳���Ǥ��롣
 *------------------------------------------------------------*/
static int TITLE_getConfigOneLine( FILE *fp, char *line, int linelen)
{
	char    buf[1024];
	int     startflg = FALSE;
	int     linenum=0;
	line[0] = '\0';
	
	while( fgets( buf, sizeof( buf ), fp )){
		linenum ++;
		if( buf[0] == '#' )continue;        /* comment */
		if( buf[0] == '\n' )continue;       /* none    */
		/*  �Ԥ���������    */
		/*  �ޤ� tab �� " " ���֤�������    */
		replaceString( buf, '\t' , ' ' );
		/* ���ڡ������ */
		deleteCharFromString( buf, " ");

		if( buf[0] == '{' ) {
			if( startflg == TRUE ) {
				print( "titleconfig:�����S�������u{�v�o�X�{�F: %d \n",linenum);
				/* } */
				return -1;
			}
			startflg = TRUE;
		}
		else if( buf[0] == '}' ) {
			if( startflg == FALSE) {
				print( "titleconfig:�����S�������u{�v�o�X�{�F: %d \n",linenum);
				/* } */
				return -1;
			}
			return 1;
		}
		else {
			/* "{"�Ǵ��˻ϤޤäƤ����� }*/
			if( startflg == TRUE ) {
				if( strlen( line) != 0 ) {
					if( line[strlen(line) -1] != ',' ) {
						strcatsafe( line, linelen, ",");
					}
				}
				/* ���ԤˤޤȤ�Ƥ���*/
				chompex( buf );
				strcatsafe( line,linelen,  buf);
			}
			/* ͭ���Ԥ���"{"�ǤϤ��ޤäƤʤ����Ϥ��Τޤޣ��Ԥ��֤� }*/
			else {
				chompex( buf );
				strcatsafe( line,linelen,  buf);
				return 1;
			}
		}
	}
	/* �����ˤ���ȸ�������EOF �ʹ�������ʤ������ɤ��� */
	return 0;
}
static int TITLE_getParamData( int readarray, int array,char *src)
{
	int cnt;
	int ret;
	char    wk[64];
	
	
	for( cnt = 1; ; cnt ++ ) {
		ret = getStringFromIndexWithDelim( src,"|",cnt,wk, sizeof(wk));
		if( ret ) {
			TITLE_ConfigTable[readarray].param[array][cnt-1] = atoi(wk);
		}
		else {
			break;
		}
	}
	if( cnt == 1 )  {
		return FALSE;
	}
	return TRUE;
}

/*------------------------------------------------------------
 * �ι�����ν�����򤹤롣
 * ����
 *  filename        char*       ����ե�����̾
 * �֤���
 *  ����    TRUE(1)
 *  ����    FALSE(0)
 *------------------------------------------------------------*/
BOOL TITLE_initTitleConfig( char* filename )
{
	FILE*   f;
	char    line[1024];
	int     linenum=0;
	int     titlecfg_readlen=0;

	f = fopen(filename,"r");
	if( f == NULL ){
		errorprint;
		return FALSE;
	}

	TITLE_titlecfgnum=0;

	/*  �ޤ�ͭ���ʹԤ����Ԥ��뤫�ɤ���Ĵ�٤�    */
	/*while( fgets( line, sizeof( line ), f ) ){}*/
	while( 1 ) {
		int rc;
		rc = TITLE_getConfigOneLine( f, line, sizeof(line));
		if( rc == 0 ) break;
		if( rc == -1 ) continue;

		TITLE_titlecfgnum++;
	}

	if( fseek( f, 0, SEEK_SET ) == -1 ){
		fprint( "Seek Error\n" );
		fclose(f);
		return FALSE;
	}

	TITLE_ConfigTable = allocateMemory( sizeof(struct tagTITLE_configTable)
								   * TITLE_titlecfgnum );
	if( TITLE_ConfigTable == NULL ){
		fprint( "Can't allocate Memory %d\n" ,
				sizeof(TITLE_ConfigTable)*TITLE_titlecfgnum);
		fclose( f );
		return FALSE;
	}
	/* ����� */
{
	int     i;
	for( i = 0; i < TITLE_titlecfgnum; i ++ ) {
		TITLE_initTitleData( i);
	}
}
	TITLE_configbuf = allocateMemory( sizeof(TITLE_CONFIGBUF) * TITLE_titlecfgnum );
	if( TITLE_configbuf == NULL ){
		fprint( "Can't allocate Memory %d\n" ,
				sizeof(TITLE_CONFIGBUF)*TITLE_titlecfgnum);
		fclose( f );
		return FALSE;
	}
	
	/*  �ޤ��ɤ�ľ��    */
	linenum = 0;
	/*while( fgets( line, sizeof( line ), f ) ){}*/
	while( 1) {
		int rc;
		rc = TITLE_getConfigOneLine( f, line, sizeof(line));
		if( rc == 0 ) break;
		if( rc == -1 ) continue;

		linenum ++;

{
		char    token[256];
		int     ret;
		int     i , j ;
		int     comppos;
		BOOL    errflg =FALSE;
		for( i = 1; ; i ++ ){
			/*  �ȡ�����򸫤�    */
			ret = getStringFromIndexWithDelim( line,",",i,token,
											   sizeof(token));
			if( ret == FALSE ){
				break;
			}
			/* ��ʸ�����Ѵ� */
			for( j = 0; j < strlen( token); j ++ ) {
				token[j] = toupper( token[j]);
			}
			/* �ι�����Ĵ�٤� */
			if( strncmp( "TITLE", token, 5) == 0 ) {
				char    buf[64];
				ret = getStringFromIndexWithDelim( token,"=",2,buf,
												   sizeof(buf));
				if( ret == FALSE) {
					fprint("Syntax Error file:%s part:%d\n",filename,linenum);
					TITLE_initTitleData( titlecfg_readlen);
					errflg = TRUE;
					break;
				}
				TITLE_ConfigTable[titlecfg_readlen].title = atoi( buf);
			}
			else {
				/* �ѥ�᡼��������ɤ� */
				for( j = 0; j < arraysizeof( TITLE_param); j ++ ) {
					if( strncmp( TITLE_param[j].paramname, 
								token, 
								strlen( TITLE_param[j].paramname)) 
						== 0 )
					{
						TITLE_ConfigTable[titlecfg_readlen].paramindex[i-1] = j;
						/* �������٥�ȤǾι������å�����ե饰��Ω�Ƥ롣 */
						if( j == 0 || j == 1 || j == 2) {
							TITLE_ConfigTable[titlecfg_readlen].equipcheckflg = TRUE;
						}
						break;
					}
				}
				/* �ѥ�᡼�����ѤʤΤ���ꤵ�줿 */
				if( j == arraysizeof( TITLE_param) ) {
					fprint("Invalid Param Name file:%s part:%d\n",
							filename,linenum);
					TITLE_initTitleData( titlecfg_readlen);
					errflg = TRUE;
					break;
				}
				/* ���桤�����椬¸�ߤ��뤫Ĵ�٤� */
				comppos = charInclude( token, "<>=");
				if( comppos == -1 ) {
					fprint("Syntax Error file:%s part:%d\n",filename,linenum);
					TITLE_initTitleData( titlecfg_readlen);
					errflg = TRUE;
					break;;
				}
				/* ���桤�����椬���İʾ�ͭ��Ȥ� */
				if( charInclude( &token[comppos+1], "<>=") != -1 ) {
					/* �ɤ����Ӥ����ɤ� */
					for( j = 0; j < 3;  j ++ ) {
						if( memcmp( TITLE_compare[j].compare, &token[comppos], 2 ) == 0 ) {
							break;
						}
					}
					if( j == 3 ) {
						fprint("Syntax Error file:%s part:%d\n",filename,linenum);
						TITLE_initTitleData( titlecfg_readlen);
						errflg = TRUE;
						break;
					}
					else {
						ret = TITLE_getParamData( titlecfg_readlen,i-1,&token[comppos+2]);
						if( !ret ) {
							fprint("Syntax Error file:%s part:%d\n",filename,linenum);
							TITLE_initTitleData( titlecfg_readlen);
							errflg = TRUE;
							break;
						}

						/*TITLE_ConfigTable[titlecfg_readlen].param[i-1] 
								= atoi( &token[comppos+2]);*/
						TITLE_ConfigTable[titlecfg_readlen].compareflg[i-1] = j;
					}
				}
				else {
					ret = TITLE_getParamData( titlecfg_readlen,i-1,&token[comppos+1]);
					if( !ret ) {
						fprint("Syntax Error file:%s part:%d\n",filename,linenum);
						TITLE_initTitleData( titlecfg_readlen);
						errflg = TRUE;
						break;
					}
					/*TITLE_ConfigTable[titlecfg_readlen].param[i-1] 
							= atoi( &token[comppos+1]);*/
					/* �ɤ����Ӥ����ɤ� */
					for( j = 3; j < 6;  j ++ ) {
						if( memcmp( TITLE_compare[j].compare, &token[comppos], 1 ) == 0 ) {
							TITLE_ConfigTable[titlecfg_readlen].compareflg[i-1] = j;
							break;
						}
					}
				}
			}
		}
		/* �ι�̾���̵꤬���ä� or �ʤ󤫤���Υ��顼*/
		if( errflg || TITLE_ConfigTable[titlecfg_readlen].title == -1 ) {
			fprint("No Title Error file:%s part:%d\n",filename,linenum);
			TITLE_initTitleData( titlecfg_readlen);
		}
		else {
			titlecfg_readlen ++;
		}
}
	}
	fclose(f);

	TITLE_titlecfgnum = titlecfg_readlen;

	print( "Valid Title Num is %d...", TITLE_titlecfgnum );

#if 0

	{
		int i, j;
		for( i=0; i < TITLE_titlecfgnum ; i++ ) {
			print( "Titlearray[%d]\n", i);
			for( j = 0; 
				j < arraysizeof( TITLE_ConfigTable[i].param) && 
				TITLE_ConfigTable[i].param[j] != -1 ; 
				j ++ ) 
			{
			   print( "name[%s] data[%d] flg[%s] ",
					   TITLE_param[TITLE_ConfigTable[i].paramindex[j]].paramname,
					   TITLE_ConfigTable[i].param[j],
					   TITLE_compare[TITLE_ConfigTable[i].compareflg[j]].compare
				   );
				if( j %2 ==0 ) print( "\n");
			}
			print( "\nTitleindex [%d] ",TITLE_ConfigTable[i].title);
			print( "equipcheckflg [%d]\n",TITLE_ConfigTable[i].equipcheckflg);
		}
	}
#endif
	return TRUE;
}
/*------------------------------------------------------------
 * �ι�����˹礦��Ĵ�٤ƾι��Ϳ���롣
 * ����
 *  charaindex        int   ����饤��ǥå���
 *  mode              BOOL  TRUE:item=�Τ�ΤΤ�Ĵ�٤� FALSE:����
 * �֤���
 *  TRUE: �ι���Ѳ�ͭ�ꡣ
 *  FALSE:����̵���ä���
 *------------------------------------------------------------*/
static BOOL TITLE_TitleCheck_Main( int charaindex, BOOL mode, int *addcnt, int *delcnt)
{
	int     i, j,k, ret;
	
	/* ����� */
{
	int     i;
	for( i = 0; i < TITLE_titlecfgnum && TITLE_configbuf[i].title != -1; i ++ ) {
		TITLE_configbuf[i].title = -1;
		TITLE_configbuf[i].flg = 0;
	}
}
	*addcnt = 0;
	*delcnt = 0;
	
	for( i = 0; i < TITLE_titlecfgnum; i ++ ) {
		int rc = TRUE;
		int title = TITLE_ConfigTable[i].title;
		if( mode == TRUE ) {
			if( TITLE_ConfigTable[i].equipcheckflg != TRUE ) {
				continue;
			}
		}
		for( j = 0; 
			j < arraysizeof( TITLE_ConfigTable[i].param) && 
			TITLE_ConfigTable[i].param[j][0] != -1 ; 
			j ++ ) 
		{
			int index = TITLE_ConfigTable[i].paramindex[j];
			if( TITLE_param[index].checkfunc != NULL ) {
				rc = TITLE_param[index].checkfunc( 
						charaindex,
						TITLE_param[index].element, 
						TITLE_ConfigTable[i].param[j],
						TITLE_ConfigTable[i].compareflg[j]
						);
				if( rc != TRUE ) break;
			}
		}
		/* ʣ������Ʊ���ι椬������Ƥ��Ƥ�����ΰ٤ˡ�
		 * ��ö�Хåե�������Ƥ��η�̤���add,deltile����
		 */
		for( k = 0; k < TITLE_titlecfgnum; k ++ ) {
			if( TITLE_configbuf[k].title == title ) { 
				if( rc ) {
					TITLE_configbuf[k].flg = 1;
				}
				break;
			}
			else if( TITLE_configbuf[k].title == -1 ) {
				TITLE_configbuf[k].title = title;
				TITLE_configbuf[k].flg = rc ? 1: -1;
				break;
			}
		}
	}
	ret = FALSE;
	for( i = 0; i < TITLE_titlecfgnum && TITLE_configbuf[i].title != -1; i ++ ) {
		if( TITLE_configbuf[i].flg == -1 ) {
			*delcnt += TITLE_deltitle( charaindex, TITLE_configbuf[i].title);
		}
		else {
			*addcnt += TITLE_addtitle( charaindex, TITLE_configbuf[i].title) ? 1:0;
		}
	}
	if( *delcnt > 0 || *addcnt > 0) {
		ret = TRUE;
	}
	return ret;
	
}
/*------------------------------------------------------------
 * �ι�����˹礦��Ĵ�٤ƾι��Ϳ���롣
 * ����
 *  charaindex        int   ����饤��ǥå���
 *  mode              BOOL  TRUE:item=�Τ�ΤΤ�Ĵ�٤� FALSE:����
 * �֤���
 *  TRUE: �ι���Ѳ�ͭ�ꡣ
 *  FALSE:����̵���ä���
 *------------------------------------------------------------*/
BOOL TITLE_TitleCheck( int charaindex, BOOL mode)
{
#define     TITLE_MSGUNIT1      "TSU"
#define     TITLE_MSGUNIT2      "KO"
	int     addcnt,delcnt;
	BOOL    rc;
	char    msgbuf[64];
	rc = TITLE_TitleCheck_Main( charaindex, mode, &addcnt,&delcnt);
	if( rc ) {
		if( delcnt > 0 ) {
			snprintf( msgbuf, sizeof( msgbuf), 
						"���h%d%s �ٸ��I", delcnt,
						delcnt < 10 ?  TITLE_MSGUNIT1:TITLE_MSGUNIT2);
			CHAR_talkToCli( charaindex, -1, msgbuf,  CHAR_COLORYELLOW);
		}
		if( addcnt > 0 ) {
			snprintf( msgbuf, sizeof( msgbuf), 
						"��o%d%s �ٸ��I", addcnt,
						addcnt < 10 ?  TITLE_MSGUNIT1:TITLE_MSGUNIT2);
			CHAR_talkToCli( charaindex, -1, msgbuf,  CHAR_COLORYELLOW);
		}
	}
	return rc;
}
/*------------------------------------------------------------
 * �ι�����˹礦��Ĵ�٤ƾι��Ϳ���롣
 * ����
 *  charaindex        int   ����饤��ǥå���
 *  mode              BOOL  TRUE:item=�Τ�ΤΤ�Ĵ�٤� FALSE:����
 * �֤���
 *  TRUE: �ι���Ѳ�ͭ�ꡣ
 *  FALSE:����̵���ä���
 *------------------------------------------------------------*/
BOOL TITLE_TitleCheck_Nomsg( int charaindex, BOOL mode, int *addcnt, int *delcnt)
{
	return( TITLE_TitleCheck_Main( charaindex, mode, addcnt,delcnt));
}


static int TITLE_IntCheck( int charaindex, int elem, int *data, int flg)
{
	int     rc = FALSE;
	int     i;
	for( i = 0; i < TITLE_PARAMSIZE && *(data+i) != -1; i ++ ) {
		switch( flg) {
		  case 0:       /* "<=" */
			if( CHAR_getInt( charaindex, elem) <= *(data+i) ) rc = TRUE;
			break;
		  case 1:       /* ">=" */
			if( CHAR_getInt( charaindex, elem) >= *(data+i) ) rc = TRUE;
			break;
		  case 2:       /* "<>" */
			if( CHAR_getInt( charaindex, elem) != *(data+i) ) rc = TRUE;
			break;
		  case 3:       /* ">" */
			if( CHAR_getInt( charaindex, elem) > *(data+i) ) rc = TRUE;
			break;
		  case 4:       /* "<"  */
			if( CHAR_getInt( charaindex, elem) < *(data+i) ) rc = TRUE;
			break;
		  case 5:       /* "=" */
			if( CHAR_getInt( charaindex, elem) == *(data+i) ) rc = TRUE;
			break;
		  default:
		  rc= FALSE;
		  break;
		}
		if( rc) break;
	}
	return rc;
}
static int TITLE_WorkIntCheck( int charaindex,int elem, int *data, int flg)
{
	int     rc = FALSE;
	int i;
	for( i = 0; i < TITLE_PARAMSIZE && *(data+i) != -1; i ++ ) {
		switch( flg) {
		  case 0:       /* "<=" */
			if( CHAR_getWorkInt( charaindex, elem) <= *(data+i) ) rc = TRUE;
			break;
		  case 1:       /* ">=" */
			if( CHAR_getWorkInt( charaindex, elem) >= *(data+i) ) rc = TRUE;
			break;
		  case 2:       /* "<>" */
			if( CHAR_getWorkInt( charaindex, elem) != *(data+i) ) rc = TRUE;
			break;
		  case 3:       /* ">" */
			if( CHAR_getWorkInt( charaindex, elem) > *(data +i)) rc = TRUE;
			break;
		  case 4:       /* "<"  */
			if( CHAR_getWorkInt( charaindex, elem) < *(data+i) ) rc = TRUE;
			break;
		  case 5:       /* "=" */
			if( CHAR_getWorkInt( charaindex, elem) == *(data+i) ) rc = TRUE;
			break;
		  default:
		  rc= FALSE;
		  break;
		}
		if( rc ) break;
	}
	return rc;
}
/* --------------------------------------
 * ���ꤵ�줿data�Ȼ��äƤ��륢���ƥ����Ӥ��롣
 * -------------------------------------*/
static int TITLE_ItemCheckMain( int charaindex, int itemhaveindex, int *data, int flg)
{
	int i,j;
	int rc = FALSE;
	
	if( flg == 2 ) rc = TRUE;

	for( j = 0; j < TITLE_PARAMSIZE && *(data+j) != -1; j ++ ) {
		for( i =0; i < itemhaveindex; i ++ ) {
			int     itemindex = CHAR_getItemIndex(charaindex,i);
			if( ITEM_CHECKINDEX(itemindex) ) {
				switch( flg) {
				  case 0:       /* "<=" */
					if( ITEM_getInt( itemindex, ITEM_ID) <= *(data+j) ) rc = TRUE;
					break;
				  case 1:       /* ">=" */
					if( ITEM_getInt( itemindex, ITEM_ID) >= *(data+j) ) rc = TRUE;
					break;
				  case 3:       /* ">" */
					if( ITEM_getInt( itemindex, ITEM_ID) > *(data +j)) rc = TRUE;
					break;
				  case 4:       /* "<"  */
					if( ITEM_getInt( itemindex, ITEM_ID) < *(data +j)) rc = TRUE;
					break;
				  case 5:       /* "=" */
					if( ITEM_getInt( itemindex, ITEM_ID) == *(data +j)) rc = TRUE;
					break;
				  /* ����������̰�����
				   * �����ƥ������򸫤Ƥ������äƤ��ʤ��ä��鿿�Ȥ��롣
				   * �����Ǥϻ��äƤ�����򵶤Ȥ��ƥ����å�
				   */
				  case 2:       /* "<>" */
					if( ITEM_getInt( itemindex, ITEM_ID) == *(data +j)) rc = FALSE;
					break;
				  default:
				  rc= FALSE;
				  break;
				}
				if( rc && flg != 2) break;
			}
		}
		if( rc) break;
	}
	return rc;
}


static int TITLE_ItemCheck( int charaindex,int elem, int *data, int flg)
{
	return( TITLE_ItemCheckMain( charaindex, CHAR_MAXITEMHAVE,data,flg));
}

static int TITLE_ItemEquipCheck( int charaindex,int elem, int *data, int flg)
{
	return( TITLE_ItemCheckMain( charaindex, CHAR_EQUIPPLACENUM,data,flg));
}

static int TITLE_SkillCheck( int charaindex,int elem, int *data, int flg)
{
	int i;
	int rc = FALSE;
	CHAR_HaveSkill* hskill;

	for( i = 0 ; i < CHAR_SKILLMAXHAVE ; i ++ ){
		hskill = CHAR_getCharHaveSkill( charaindex, i );
		if( hskill != NULL && hskill->use == TRUE ) {

			if( *(data + 1) != -2 ) {
				if( SKILL_getInt( &hskill->skill, SKILL_IDENTITY) == *data) {
					rc = TRUE;
					break;
				}
			}

			else {
				if( SKILL_getInt( &hskill->skill, SKILL_IDENTITY) == *data) {
					rc = FALSE;
					break;
				}
				else {
					rc = TRUE;
				}
			}
		}
	}
	if( rc ) {

		if( *(data+1) != -1 && *(data+1) != -2) {
			rc = FALSE;
			switch( flg) {
			  case 0:       /* "<=" */
				if( SKILL_getInt( &hskill->skill, SKILL_LEVEL) <= *(data+1) ) rc = TRUE;
				break;
			  case 1:       /* ">=" */
				if( SKILL_getInt( &hskill->skill, SKILL_LEVEL) >= *(data+1) ) rc = TRUE;
				break;
			  case 2:       /* "<>" */
				if( SKILL_getInt( &hskill->skill, SKILL_LEVEL) != *(data+1) ) rc = TRUE;
				break;
			  case 3:       /* ">" */
				if( SKILL_getInt( &hskill->skill, SKILL_LEVEL) > *(data +1)) rc = TRUE;
				break;
			  case 4:       /* "<"  */
				if( SKILL_getInt( &hskill->skill, SKILL_LEVEL) < *(data+1) ) rc = TRUE;
				break;
			  case 5:       /* "=" */
				if( SKILL_getInt( &hskill->skill, SKILL_LEVEL) == *(data+1) ) rc = TRUE;
				break;
			  default:
			  rc= FALSE;
			  break;
			}
		}
	}
	return rc;
}
/* --------------------------------------
 * �˥���餫������餫�����å����롣
 * -------------------------------------*/
static int TITLE_SexCheck( int charaindex,int elem, int *data, int flg)
{
	int     i,j;
	struct {
		int sex[25];
	}s_sex[] =
	{ { { 30008,30009,30010,30011,30012,30013,30014,30015,
		30020,30021,30022,30023,30024,30025,30026,30027,
		30028,30029,30030,30031,30032,30033,30034,30035,-1}
	  },
	  { {30000,30001,30002,30003,30004,30005,30006,30007,
		30016,30017,30018,30019,30036,30037,30038,30039,
		-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  -1}
	  }
	};
	for( i = 0; i < 2; i ++ ) {
		for( j = 0; s_sex[i].sex[j] != -1 ; j ++ ) {
			if( CHAR_getInt( charaindex, CHAR_BASEBASEIMAGENUMBER ) 
				== s_sex[i].sex[j] ) 
			{
				if( i == *data ) return TRUE;
				else            return FALSE;
			}
		}
	}
	return FALSE;
}

