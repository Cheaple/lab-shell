/* This file contains the code for parser used to parse the input
 * given to shell program. You shouldn't need to modify this
 * file */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "parse.h"

#define PIPE ('|')
#define BG ('&')
#define RIN ('<')
#define RUT ('>')

#define ispipe(c) ((c) == PIPE)
#define isbg(c) ((c) == BG)
#define isrin(c) ((c) == RIN)
#define isrut(c) ((c) == RUT)
#define isspec(c) (ispipe(c) || isbg(c) || isrin(c) || isrut(c))

static Pgm cmdbuf[20], *cmds;
static char cbuf[256], *cp;
static char *pbuf[50], **pp;

int parse(char *buf, Command *c)
{
    int n;
    Pgm *cmd0;

    char *t = buf; // Cursur for input string buf
    char *tok;

    init();
    c->rstdin = NULL;
    c->rstdout = NULL;
    c->rstderr = NULL;
    c->background = false;
    c->pgm = NULL;

newcmd:
    if ((n = acmd(t, &cmd0)) <= 0) // Border condition, no valid command
    {
        return -1;
    }

    t += n;

    cmd0->next = c->pgm; // Add a Pgm node on the head of Command linked list c
    c->pgm = cmd0;

newtoken:
    n = nexttoken(t, &tok);
    if (n == 0)
    {
        return 1;
    }
    t += n;

    switch (*tok)
    {
    case PIPE:
        goto newcmd;
    case BG:
        n = nexttoken(t, &tok);
        if (n == 0)
        {
            c->background = 1;
            return 1;
        }
        else
        {
            fprintf(stderr, "illegal bakgrounding\n");
            return -1;
        }
    case RIN:
        if (c->rstdin != NULL)
        {
            fprintf(stderr, "duplicate redirection of stdin\n");
            return -1;
        }
        if ((n = nexttoken(t, &(c->rstdin))) < 0)
        {
            return -1;
        }
        if (!isidentifier(c->rstdin))
        {
            fprintf(stderr, "Illegal filename: \"%s\"\n", c->rstdin);
            return -1;
        }
        t += n;
        goto newtoken;
    case RUT:
        if (c->rstdout != NULL)
        {
            fprintf(stderr, "duplicate redirection of stdout\n");
            return -1;
        }
        if ((n = nexttoken(t, &(c->rstdout))) < 0)
        {
            return -1;
        }
        if (!isidentifier(c->rstdout))
        {
            fprintf(stderr, "Illegal filename: \"%s\"\n", c->rstdout);
            return -1;
        }
        t += n;
        goto newtoken;
    default:
        return -1;
    }
}

/*
 * Initialize the global command buffers.
 */
void init(void)
{
    int i;
    for (i = 0; i < 19; i++)
    {
        cmdbuf[i].next = &cmdbuf[i + 1];
    }
    cmdbuf[19].next = NULL;
    cmds = cmdbuf;
    cp = cbuf;
    pp = pbuf;
}

/*
 * For input string s, the first word will be regarded as command itself.
 * return the length of token.
 */
int nexttoken(char *s, char **tok)
{
    char *s0 = s;
    char c;

    *tok = cp;
    while (isspace(c = *s++) && c) // Remove leading whitespace from s
        ;
    if (c == '\0') // Empty string, exit
    {
        return 0;
    }
    if (isspec(c)) // Special character ('|', '&', '>', '<'), store it
    {
        *cp++ = c;
        *cp++ = '\0';
    }
    else // Normal Command
    {
        *cp++ = c;
        do
        {
            c = *cp++ = *s++;
        } while (!isspace(c) && !isspec(c) && (c != '\0'));
        --s;
        --cp;
        *cp++ = '\0';
    }
    return (int)(s - s0); // Greater than 0, and cp is updated. Process it.
}

int acmd(char *s, Pgm **cmd)
{
    char *tok;
    int n, cnt = 0;
    Pgm *cmd0 = cmds;   // Current *cmds will be occupied.
    cmds = cmds->next;  // cmds jummp to the next pointer.
    cmd0->next = NULL;  // and set to NULL to wait later modification.
    cmd0->pgmlist = pp; // Current cmds->phmlist = pp.

next:
    n = nexttoken(s, &tok);
    if (n == 0 || isspec(*tok)) // Border condition, token is empty string or special character.
    {
        *cmd = cmd0;  // cmd jump back.
        *pp++ = NULL; // Set pp as NULL to divide different commands.
        return cnt;
    }
    else // Store command and its options into pp.
    {
        *pp++ = tok;
        cnt += n;
        s += n;
        goto next;
    }
}

#define IDCHARS "_-.,/~+"

int isidentifier(char *s)
{
    while (*s)
    {
        char *p = strrchr(IDCHARS, *s);
        if (!isalnum(*s++) && (p == NULL))
            return 0;
    }
    return 1;
}

/* Print a (linked) list of Pgm:s.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void PrintPgm(Pgm *p)
{
    if (p == NULL)
    {
        return;
    }
    else
    {
        char **pl = p->pgmlist;

        /* The list is in reversed order so print
         * it reversed to get right
         */
        PrintPgm(p->next);
        printf("            * [ ");
        while (*pl)
        {
            printf("%s ", *pl++);
        }
        printf("]\n");
    }
}

/*
 * Print a Command structure as returned by parse on stdout.
 *
 * Helper function, no need to change. Might be useful to study as inpsiration.
 */
void DebugPrintCommand(Command *cmd)
{
    printf("------------------------------\n");
    printf("Parse OK\n");
    printf("stdin:      %s\n", cmd->rstdin ? cmd->rstdin : "<none>");
    printf("stdout:     %s\n", cmd->rstdout ? cmd->rstdout : "<none>");
    printf("background: %s\n", cmd->background ? "true" : "false");
    printf("Pgms:\n");
    PrintPgm(cmd->pgm);
    printf("------------------------------\n");
}
