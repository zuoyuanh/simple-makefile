#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#define HASH_TABLE_SIZE 29
#define DEFAULT_HASH_LENGTH 5
#define DEFAULT_LIST_LENGTH 10
#define DEFAULT_LINE_LENGTH 10

struct rule
{
   int dep;
   int act;
   char *key;
   char *original;
   char **actions;
   char **dependencies;
   struct rule *next;
};

void *checked_malloc(size_t size)
{
   void *p = malloc(size);
   if (!p) {
      perror("");
      exit(-1);
   }
   return p;
}

void *checked_realloc(void *p, size_t size)
{
   void *q = realloc(p, size);
   if (!q) {
      perror("");
      exit(-1);
   }
   return q;
}

char *readline(FILE *f, int *eof)
{
   char c;
   int size = 0;
   int nonspace_found = 0;
   int str_size = DEFAULT_LINE_LENGTH;
   char *tmp;
   char *line = checked_malloc(str_size * sizeof(char));
   while ((c = fgetc(f))) {
      if (c == EOF) {
         *eof = 1;
         break;
      }
      if (c == '\n') break;
      if (size+1 >= str_size) {
         str_size *= 2;
         line = checked_realloc(line, str_size * sizeof(char));
      }
      line[size++] = c;
   }
   line[size] = '\0';
   tmp = line;
   while (*tmp != '\0') {
      if (!isspace(*tmp)) {
         nonspace_found++;
         break;
      }
      tmp++;
   }
   if (size == 0) {
      free(line);
      line = NULL;
   }
   if (size!=0 && !nonspace_found) {
      free(line);
      line = NULL;
   }
   return line;
}

#define NO_COL_ERR -1
#define MULT_COL_ERR -2
#define NO_RULE_MAKE_TGT -3
#define EMPTY_SMKFILE -4
#define ONE_RULE_MULT_TGTS -5
#define ACT_BEFORE_RUL_ERR -6
#define SYS_CALL_ERR -7

void error_msg(int errorno)
{
   switch (errorno)
   {
      case NO_COL_ERR: 
         fprintf(stderr, "Error: no colon found in a rule. \n");
         break;
      case MULT_COL_ERR:
         fprintf(stderr, "Error: multiple colons in a rule. \n");
         break;
      case EMPTY_SMKFILE:
         fprintf(stderr, "Error: empty Smakefile.\n");
         break;
      case NO_RULE_MAKE_TGT:
         fprintf(stderr, "Error: No rule to make target\n");
         break;
      case ONE_RULE_MULT_TGTS:
         fprintf(stderr, "Error: one rule with multiple targets. \n");
         break;
      case ACT_BEFORE_RUL_ERR:
         fprintf(stderr, "Error: actions found before rules. \n");
         break;
      case SYS_CALL_ERR:
         fprintf(stderr, "Error: system reported an error while executing commands. \n");
         break;
   }
}

int isrule(char *line, struct rule **hash_table)
{
   int i = 0;
   int cnt = 0;
   if (line == NULL)
      return 0;
   if (line[0] != '\t') {
      while (line[i] != '\0') {
         if (line[i] == ':')
            cnt++;
         i++;
      }
      if (cnt == 1)
         return 1;
      else if (cnt > 1)
         return MULT_COL_ERR;
      else if (cnt == 0)
         return NO_COL_ERR;
   }
   return 0;
}

void free_rule(struct rule *r)
{
   int i;
   if (r == NULL) return;
   if (r->original)
      free(r->original);
   if (r->dependencies)
      free(r->dependencies);
   if (r->actions) {
      for (i=0; i<r->act; i++)
         free((r->actions)[i]);
      free(r->actions);
   }
   free(r);
}

char *my_strsep(char **stringp, char *delim)
{
   int white_space;
   char *tok;
   white_space = strcmp(delim, "WS");
   if (!stringp || !*stringp)
      return NULL;
   tok = *stringp;
   while (**stringp != '\0') {
      if (white_space == 0) {
         if (isspace(**stringp)) {
            **stringp = '\0';
            (*stringp)++;
            return tok;
         }
      } else if (**stringp == *delim) {
         **stringp = '\0';
         (*stringp)++;
         return tok;
      }
      (*stringp)++;
   }
   *stringp = NULL;
   return tok;
}

int min(int a, int b)
{
   return (a<b) ? a : b;
}

int hash_code(char *key)
{
   int i;
   int hash = 0;
   int len = min(DEFAULT_HASH_LENGTH, strlen(key));
   if (!key) return -1;
   for (i=0; i<len; i++)
      hash = 37 * hash + (int)key[i];
   return hash % HASH_TABLE_SIZE;
}

void add_to_table(struct rule *r, struct rule **hash_table)
{
   int hash = hash_code(r->key);
   if (r == NULL) return;
   if (hash_table[hash] == NULL)
      hash_table[hash] = r;
   else {
      struct rule *curr = hash_table[hash];
      while (curr->next != NULL)
         curr = curr->next;
      curr->next = r;
   }
}

struct rule *find_from_table(char *key, struct rule **hash_table)
{
   struct rule *curr = hash_table[hash_code(key)];
   if (curr == NULL) {
      return NULL;
   } else {
      while (curr != NULL) {
         if (strcmp(curr->key, key) == 0)
            break;
         curr = curr->next;
      }
   }
   return curr;
}

void free_table(struct rule **hash_table)
{
   int i;
   for (i=0; i<HASH_TABLE_SIZE; i++) {
      struct rule *curr = hash_table[i];
      while (curr != NULL) {
         struct rule *next = curr->next;
         free_rule(curr);
         curr = next;
      }
   }
   free(hash_table);
}

int execute_action(char *action)
{
   while (*action==' ' || *action=='\t')
      action++;
   printf("%s\n", action);
   if (system(action) != 0)
      return SYS_CALL_ERR;
   return 0;
}

void execute_actions_set(struct rule *r, struct rule **hash_table)
{
   int i;
   for (i=0; i<r->act; i++) {
      if (execute_action((r->actions)[i]) == SYS_CALL_ERR) {
         error_msg(SYS_CALL_ERR);
         free_table(hash_table);
         exit(-1);
      }
   }
}

int traverse(char *key, struct rule **hash_table)
{
   int i;
   int tmp, tmp1;
   int need_to_exec = 0;
   struct stat buf;
   struct rule *r = find_from_table(key, hash_table);
   if (r == NULL)
      return -1;
   if (stat(key, &buf))
      need_to_exec = 1;
   if (r->dep == 0) {
      execute_actions_set(r, hash_table);
      return 0;
   } else {
      for (i=0; i<r->dep; i++) {
         struct stat buf2;
         char *dependency = (r->dependencies)[i];
         tmp = traverse(dependency, hash_table);
         tmp1 = stat(dependency, &buf2);
         if (tmp && tmp1) {
            error_msg(NO_RULE_MAKE_TGT);
            free_table(hash_table);
            exit(-1);
         }
         if (need_to_exec==0 && buf2.st_mtime>buf.st_mtime)
            need_to_exec = 1;
      }
      if (need_to_exec)
         execute_actions_set(r, hash_table);
   }
   return 0;
}

int process_key(char **res, char *tar_list)
{
   char *key;
   char *tmps;
   char *tmp = tar_list;
   int first_space_found = 0;
   while (isspace(*tmp)) tmp++;
   key = tmp;
   tmps = tmp;
   while (tmps && *tmps != '\0') {
      if (isspace(*tmps)) {
         first_space_found = 1;
         *tmps = '\0';
      } else {
         if (!isspace(*tmps) && first_space_found)
            return ONE_RULE_MULT_TGTS;
      }
      tmps++;
   }
   *res = key;
   return 0;
}

int process_dep(char ***res, char *dep_list)
{
   int dep = 0;
   int dep_sz = DEFAULT_LIST_LENGTH;
   char *tok;
   char *tmp = dep_list;
   char **dependencies = checked_malloc(dep_sz * sizeof(char *));
   while ((tok = my_strsep(&tmp, "WS"))) {
      if (strlen(tok) != 0) {
         if (dep+1 > dep_sz) {
            dep_sz *= 2;
            dependencies = checked_realloc(dependencies, dep_sz * sizeof(char *));
         }
         dependencies[dep++] = tok;
      }
   }
   *res = dependencies;
   return dep;
}

struct rule *process_rule(FILE *f, char *line, struct rule **hash_table, int *end)
{
   int dep;
   int errno;
   char *key;
   char *tmp;
   char *original;
   char **dependencies = NULL;
   struct rule *new_rule = NULL;
   original = line;
   tmp = line;
   tmp = my_strsep(&line, ":");
   if ((errno = process_key(&key, tmp))) {
      error_msg(errno);
      if (original) free(original);
      free_table(hash_table);
      if (new_rule) free(new_rule);
      fclose(f);
      exit(-1);
   }
   if (key == NULL) {
      *end = 1;
      free(original);
      return NULL;
   }
   if (strlen(key) == 0) {
      free(original);
      if (dependencies)
         free(dependencies);
      return NULL;
   }
   tmp = my_strsep(&line, "\0");
   dep = process_dep(&dependencies, tmp);
   new_rule = malloc(sizeof(struct rule));
   new_rule->original = original;
   new_rule->key = key;
   new_rule->dep = dep;
   new_rule->dependencies = dependencies;
   return new_rule;
}

void validate_rule(int ir_ret, FILE *f, char *line, struct rule **hash_table)
{
   if (ir_ret!=0 && ir_ret!=1) {
      if (ir_ret == NO_COL_ERR)
         error_msg(NO_COL_ERR);
      else if (ir_ret == MULT_COL_ERR)
         error_msg(MULT_COL_ERR);
      if (line) free(line);
      free_table(hash_table);
      fclose(f);
      exit(-1);
   }
}

struct rule *process_lines(FILE *f, char **buffer, int *end, struct rule **hash_table)
{
   int act = 0;
   int eof = 0;
   int ir_ret = 0;
   int first_line = 1;
   int act_sz = DEFAULT_LIST_LENGTH;
   char *line = *buffer;
   char **actions = NULL;
   struct rule *new_rule = NULL;
   if (line == NULL) line = readline(f, &eof);
   while (first_line || (!(isrule(line, hash_table)))) {
      *end = eof;
      ir_ret = isrule(line, hash_table);
      validate_rule(ir_ret, f, line, hash_table);
      if (!first_line || ir_ret==1)
         first_line = 0;
      if (line == NULL) {
         if (eof) break;
         line = readline(f, &eof);
         continue;
      }
      if (first_line && ir_ret==0) {
         error_msg(ACT_BEFORE_RUL_ERR);
         if (line) free(line);
         free_table(hash_table);
         fclose(f);
         exit(-1);
      }
      if (isrule(line, hash_table)) {
         new_rule = process_rule(f, line, hash_table, end);
      } else if (new_rule != NULL) {
         if (!actions)
            actions = checked_malloc(act_sz * sizeof(char *));
         if (act+1 > act_sz) {
            act_sz *= 2;
            actions = checked_realloc(actions, act_sz * sizeof(char *));
         }
         actions[act++] = line;
      }
      if (eof) break;
      line = readline(f, &eof);
   }
   if (new_rule == NULL)
      return NULL;
   new_rule->act = act;
   new_rule->actions = actions;
   new_rule->next = NULL;
   *buffer = line;
   return new_rule;
}

char *process_file(FILE *f, struct rule **hash_table)
{
   int read_end = 0;
   int found_first = 0;
   char *buffer = NULL;
   char *first_line = NULL;
   struct rule *r = NULL;
   while ((r = process_lines(f, &buffer, &read_end, hash_table)) || !read_end) {
      if (r != NULL) {
         if (!found_first) {
            found_first = 1;
            first_line = r->key;
         }
         add_to_table(r, hash_table);
      }
   }
   return first_line;
}

int main(int argc, char *argv[])
{
   int i;
   char *tmp;
   char *start;
   struct rule **hash_table;
   FILE *f = fopen("Smakefile", "r");
   if (!f) {
      perror("Smakefile");
      exit(-1);
   }
   hash_table = checked_malloc(HASH_TABLE_SIZE * sizeof(struct rule *));
   for (i=0; i<HASH_TABLE_SIZE; i++)
      hash_table[i] = NULL;
   tmp = process_file(f, hash_table);
   fclose(f);
   if (tmp == NULL) {
      error_msg(EMPTY_SMKFILE);
      free_table(hash_table);
      exit(-1);
   }
   if (argc == 1) start = tmp;
   else start = argv[1];
   if (traverse(start, hash_table)) {
      error_msg(NO_RULE_MAKE_TGT);
      free_table(hash_table);
      exit(-1);
   }
   free_table(hash_table);
   return 0;
}
