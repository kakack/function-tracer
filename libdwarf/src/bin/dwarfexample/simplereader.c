#include <config.h>

#include <stdio.h>  /* fprintf() printf() snprintf() */
#include <stdlib.h> /* atoi() exit() free() */
#include <string.h> /* memset() strcmp() strcpy() strdup()
    strlen() strncmp() */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#ifdef _WIN32
#include <io.h> /* close() open() */
#elif defined HAVE_UNISTD_H
#include <unistd.h> /* close() */
#endif /* _WIN32 */

#ifdef HAVE_FCNTL_H
#include <fcntl.h> /* open() O_RDONLY */
#endif /* HAVE_FCNTL_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"

#ifndef O_RDONLY
/*  This is for a Windows environment */
# define O_RDONLY _O_RDONLY
#endif

#ifdef _O_BINARY
/*  This is for a Windows environment */
#define O_BINARY _O_BINARY
#else
# ifndef O_BINARY
# define O_BINARY 0  /* So it does nothing in Linux/Unix */
# endif
#endif /* O_BINARY */

struct srcfilesdata {
    char ** srcfiles;
    Dwarf_Signed srcfilescount;
    int srcfilesres;
};

static void read_cu_list(Dwarf_Debug dbg);
static void print_die_data(Dwarf_Debug dbg, Dwarf_Die print_me,
    int level,
    struct srcfilesdata *sf);
static void get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,
    int is_info, int in_level,
    struct srcfilesdata *sf);
static void resetsrcfiles(Dwarf_Debug dbg,struct srcfilesdata *sf);

static int unittype      = DW_UT_compile;
static Dwarf_Bool g_is_info = TRUE;

int cu_version_stamp = 0;
int cu_offset_size = 0;

/*  dienumberr is used to count DIEs.
    The point is to match fissionfordie. */
static int dienumber = 0;
static int fissionfordie = -1;
static int passnullerror = 0;

#define DUPSTRARRAYSIZE 100
static const char *dupstrarray[DUPSTRARRAYSIZE];
static unsigned    dupstrused;

static void
cleanupstr(void)
{
    unsigned i = 0;
    for (i = 0; i < dupstrused; ++i) {
        free((char *)dupstrarray[i]);
        dupstrarray[i] = 0;
    }
    dupstrused = 0;
}

static void
format_sig8_string(Dwarf_Sig8*data, char* str_buf,unsigned
    buf_size)
{
    unsigned i = 0;
    char *cp = str_buf;
    if (buf_size <  19) {
        printf("FAIL: internal coding error in test.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(cp,"0x");
    cp += 2;
    buf_size -= 2;
    for (; i < sizeof(data->signature); ++i,cp+=2,buf_size--) {
        snprintf(cp, buf_size, "%02x",
            (unsigned char)(data->signature[i]));
    }
    return;
}

static void
print_debug_fission_header(struct Dwarf_Debug_Fission_Per_CU_s *fsd)
{
    const char * fissionsec = ".debug_cu_index";
    unsigned i  = 0;
    char str_buf[30];

    if (!fsd || !fsd->pcu_type) {
        /* No fission data. */
        return;
    }
    printf("\n");
    if (!strcmp(fsd->pcu_type,"tu")) {
        fissionsec = ".debug_tu_index";
    }
    printf("  %-19s = %s\n","Fission section",fissionsec);
    printf("  %-19s = 0x%"  DW_PR_XZEROS DW_PR_DUx "\n",
        "Fission index ",
        fsd->pcu_index);
    format_sig8_string(&fsd->pcu_hash,str_buf,sizeof(str_buf));
    printf("  %-19s = %s\n","Fission hash",str_buf);
    /* 0 is always unused. Skip it. */
    printf("  %-19s = %s\n","Fission entries","offset     "
        "size        DW_SECTn");
    for ( i = 1; i < DW_FISSION_SECT_COUNT; ++i)  {
        const char *nstring = 0;
        Dwarf_Unsigned off = 0;
        Dwarf_Unsigned size = fsd->pcu_size[i];
        int res = 0;
        if (size == 0) {
            continue;
        }
        res = dwarf_get_SECT_name(i,&nstring);
        if (res != DW_DLV_OK) {
            nstring = "Unknown SECT";
        }
        off = fsd->pcu_offset[i];
        printf("  %-19s = 0x%" DW_PR_XZEROS DW_PR_DUx
            " 0x%" DW_PR_XZEROS DW_PR_DUx " %2u\n",
            nstring,
            off,
            size,i);
    }
}

int
main(int argc, char **argv)
{
    Dwarf_Debug dbg = 0;
    const char *filepath = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Error error;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;
    Dwarf_Error *errp  = 0;
    int simpleerrhand = 0;
    int i = 0;
    #define MACHO_PATH_LEN 2000
    char macho_real_path[MACHO_PATH_LEN];

    macho_real_path[0] = 0;
    
    filepath = argv[i];
   
    errp = &error;

    res = dwarf_init_path(filepath,
        macho_real_path,
        MACHO_PATH_LEN,
        DW_GROUPNUMBER_ANY,errhand,errarg,&dbg, errp);

    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            if(!passnullerror) {
                dwarf_dealloc_error(dbg,error);
            }
            error = 0;
        }
        printf("Giving up, cannot do DWARF processing %s\n",
            filepath?filepath:"");
        cleanupstr();
        exit(EXIT_FAILURE);
    }
    read_cu_list(dbg);
    res = dwarf_finish(dbg);
    if (res != DW_DLV_OK) {
        printf("dwarf_finish failed!\n");
    }

    cleanupstr();
    return 0;
}

static void
read_cu_list(Dwarf_Debug dbg)
{
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half     address_size = 0;
    Dwarf_Half     version_stamp = 0;
    Dwarf_Half     offset_size = 0;
    Dwarf_Half     extension_size = 0;
    Dwarf_Sig8     signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Half     header_cu_type = unittype;
    Dwarf_Bool     is_info = g_is_info;
    Dwarf_Error error;
    int cu_number = 0;
    Dwarf_Error *errp  = 0;

    for (;;++cu_number) {
        Dwarf_Die no_die = 0;
        Dwarf_Die cu_die = 0;
        int res = DW_DLV_ERROR;
        struct srcfilesdata sf;
        sf.srcfilesres = DW_DLV_ERROR;
        sf.srcfiles = 0;
        sf.srcfilescount = 0;
        memset(&signature,0, sizeof(signature));

        if (passnullerror) {
            errp = 0;
        } else {
            errp = &error;
        }
        res = dwarf_next_cu_header_d(dbg,is_info,&cu_header_length,
            &version_stamp, &abbrev_offset,
            &address_size, &offset_size,
            &extension_size,&signature,
            &typeoffset, &next_cu_header,
            &header_cu_type,errp);
        if (res == DW_DLV_ERROR) {
            char *em = errp?dwarf_errmsg(error):
                "An error next cu her";
            printf("Error in dwarf_next_cu_header: %s\n",em);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done. */
            return;
        }
        cu_version_stamp = version_stamp;
        cu_offset_size   = offset_size;
        /* The CU will have a single sibling, a cu_die. */
        res = dwarf_siblingof_b(dbg,no_die,is_info,
            &cu_die,errp);
        if (res == DW_DLV_ERROR) {
            char *em = errp?dwarf_errmsg(error):"An error";
            printf("Error in dwarf_siblingof_b on CU die: %s\n",em);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Impossible case. */
            printf("no entry! in dwarf_siblingof on CU die \n");
            exit(EXIT_FAILURE);
        }
        get_die_and_siblings(dbg,cu_die,is_info,0,&sf);
        dwarf_dealloc(dbg,cu_die,DW_DLA_DIE);
        resetsrcfiles(dbg,&sf);
    }
}

static void
get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,
    int is_info,int in_level,
    struct srcfilesdata *sf)
{
    int res = DW_DLV_ERROR;
    Dwarf_Die cur_die=in_die;
    Dwarf_Die child = 0;
    Dwarf_Error error = 0;
    Dwarf_Error *errp = 0;

    if (passnullerror) {
        errp = 0;
    } else {
        errp = &error;
    }
    print_die_data(dbg,in_die,in_level,sf);

    for (;;) {
        Dwarf_Die sib_die = 0;
        res = dwarf_child(cur_die,&child,errp);
        if (res == DW_DLV_ERROR) {
            printf("Error in dwarf_child , level %d \n",in_level);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_OK) {
            get_die_and_siblings(dbg,child,is_info,
                in_level+1,sf);
            /* No longer need 'child' die. */
            dwarf_dealloc(dbg,child,DW_DLA_DIE);
            child = 0;
        }
        /* res == DW_DLV_NO_ENTRY or DW_DLV_OK */
        res = dwarf_siblingof_b(dbg,cur_die,is_info,&sib_die,errp);
        if (res == DW_DLV_ERROR) {
            char *em = errp?dwarf_errmsg(error):"Error siblingof_b";
            printf("Error in dwarf_siblingof_b , level %d :%s \n",
                in_level,em);
            exit(EXIT_FAILURE);
        }
        if (res == DW_DLV_NO_ENTRY) {
            /* Done at this level. */
            break;
        }
        /* res == DW_DLV_OK */
        if (cur_die != in_die) {
            dwarf_dealloc(dbg,cur_die,DW_DLA_DIE);
            cur_die = 0;
        }
        cur_die = sib_die;
        print_die_data(dbg,cur_die,in_level,sf);
    }
    return;
}

static void
get_addr(Dwarf_Attribute attr,Dwarf_Addr *val)
{
    Dwarf_Error error = 0;
    int res;
    Dwarf_Addr uval = 0;
    Dwarf_Error *errp  = 0;

    if (passnullerror) {
        errp = 0;
    } else {
        errp = &error;
    }

    res = dwarf_formaddr(attr,&uval,errp);
    if (res == DW_DLV_OK) {
        *val = uval;
        return;
    }
    return;
}

static void
print_subprog(Dwarf_Debug dbg,Dwarf_Die die,
    int level,
    struct srcfilesdata *sf,
    const char *name)
{
    int res;
    Dwarf_Error error = 0;
    Dwarf_Attribute *attrbuf = 0;
    Dwarf_Addr lowpc = 0;
    Dwarf_Addr highpc = 0;
    Dwarf_Signed attrcount = 0;
    Dwarf_Signed i;
    Dwarf_Error *errp = 0;

    if (passnullerror) {
        errp = 0;
    } else {
        errp = &error;
    }
    res = dwarf_attrlist(die,&attrbuf,&attrcount,errp);
    if (res != DW_DLV_OK) {
        if (res == DW_DLV_ERROR) {
            if (!passnullerror) {
                dwarf_dealloc_error(dbg,error);
                error = 0;
                exit(EXIT_FAILURE);
            }
        }
        return;
    }
    for (i = 0; i < attrcount ; ++i) {
        Dwarf_Half aform;
        res = dwarf_whatattr(attrbuf[i],&aform,errp);
        if (res == DW_DLV_OK) {
            if (aform == DW_AT_low_pc) {
                get_addr(attrbuf[i],&lowpc);
            }
            if (aform == DW_AT_high_pc) {
                /*  This will FAIL with DWARF4 highpc form
                    of 'class constant'.  */
                get_addr(attrbuf[i],&highpc);
            }
        }
        if (res == DW_DLV_ERROR && !passnullerror) {
            dwarf_dealloc_error(dbg,error);
            error = 0;
        }
        dwarf_dealloc(dbg,attrbuf[i],DW_DLA_ATTR);
    }
    /*  Here let's test some alternative interfaces for
        high and low pc. */

    int hresb = 0;
    int lres = 0;
    Dwarf_Addr althipcb = 0;
    Dwarf_Addr altlopc = 0;
    Dwarf_Half highform = 0;
    enum Dwarf_Form_Class highclass = 0;

    /* Should work for all DWARF DW_AT_high_pc.  */
    hresb = dwarf_highpc_b(die,&althipcb,&highform,
        &highclass,errp);
    lres = dwarf_lowpc(die,&altlopc,errp);

    if (hresb == DW_DLV_OK && lres == DW_DLV_OK) {
        althipcb += altlopc;
        printf("function name: %s ", name);
        printf("lowpc    0x%" DW_PR_XZEROS DW_PR_DUx " ",
            altlopc);
        printf("highpcb  0x%" DW_PR_XZEROS DW_PR_DUx " ",
                althipcb);
        printf("\n");
    }
    dwarf_dealloc(dbg,attrbuf,DW_DLA_LIST);
}

static void
resetsrcfiles(Dwarf_Debug dbg,struct srcfilesdata *sf)
{
    Dwarf_Signed sri = 0;
    if (sf->srcfiles) {
        for (sri = 0; sri < sf->srcfilescount; ++sri) {
            dwarf_dealloc(dbg, sf->srcfiles[sri],
                DW_DLA_STRING);
        }
        dwarf_dealloc(dbg, sf->srcfiles, DW_DLA_LIST);
    }
    sf->srcfilesres = DW_DLV_ERROR;
    sf->srcfiles = 0;
    sf->srcfilescount = 0;
}

static void
print_die_data_i(Dwarf_Debug dbg, Dwarf_Die print_me,
    int level,
    struct srcfilesdata *sf)
{
    char *name = 0;
    Dwarf_Error error = 0;
    Dwarf_Half tag = 0;
    const char *tagname = 0;
    int res = 0;
    Dwarf_Error *errp = 0;
    Dwarf_Attribute attr = 0;
    Dwarf_Half formnum = 0;
    const char *formname = 0;

    if (passnullerror) {
        errp = 0;
    } else {
        errp = &error;
    }
    res = dwarf_diename(print_me,&name,errp);
    if (res == DW_DLV_ERROR) {
        printf("Error in dwarf_diename , level %d \n",level);
        exit(EXIT_FAILURE);
    }
    if (res == DW_DLV_NO_ENTRY) {
        name = "<no DW_AT_name attr>";
    }
    res = dwarf_tag(print_me,&tag,errp);
    if (res != DW_DLV_OK) {
        printf("Error in dwarf_tag , level %d \n",level);
        exit(EXIT_FAILURE);
    }
    res = dwarf_get_TAG_name(tag,&tagname);
    if (res != DW_DLV_OK) {
        printf("Error in dwarf_get_TAG_name , level %d \n",level);
        exit(EXIT_FAILURE);
    }

    res = dwarf_attr(print_me,DW_AT_name,&attr, errp);
    if (res != DW_DLV_OK) {
        /* do nothing */
    } else {
        res = dwarf_whatform(attr,&formnum,errp);
        if (res != DW_DLV_OK) {
            printf("Error in dwarf_whatform , level %d \n",level);
            exit(EXIT_FAILURE);
        }
        formname = "form-name-unavailable";
        res = dwarf_get_FORM_name(formnum,&formname);
        if (res != DW_DLV_OK) {
            formname = "UNKNoWn FORM!";
        }
        dwarf_dealloc(dbg,attr,DW_DLA_ATTR);
    }
    
    if (tag == DW_TAG_subprogram && name != "<no DW_AT_name attr>") {
        print_subprog(dbg,print_me,level,sf,name);
    }
}

static void
print_die_data(Dwarf_Debug dbg, Dwarf_Die print_me,
    int level,
    struct srcfilesdata *sf)
{

    if (fissionfordie != -1) {
        Dwarf_Debug_Fission_Per_CU percu;
        memset(&percu,0,sizeof(percu));
        if (fissionfordie == dienumber) {
            int res = 0;
            Dwarf_Error error = 0;
            Dwarf_Error *errp = 0;

            if (passnullerror) {
                errp = 0;
            } else {
                errp = &error;
            }
            res = dwarf_get_debugfission_for_die(print_me,
                &percu,errp);
            if (res == DW_DLV_ERROR) {
                printf("FAIL: Error in "
                    "dwarf_get_debugfission_for_die %d\n",
                    fissionfordie);
                exit(EXIT_FAILURE);
            }
            if (res == DW_DLV_NO_ENTRY) {
                printf("FAIL: no-entry in "
                    "dwarf_get_debugfission_for_die %d\n",
                    fissionfordie);
                exit(EXIT_FAILURE);
            }
            print_die_data_i(dbg,print_me,level,sf);
            print_debug_fission_header(&percu);
            exit(0);
        }
        dienumber++;
        return;
    }
    print_die_data_i(dbg,print_me,level,sf);
    dienumber++;
}
