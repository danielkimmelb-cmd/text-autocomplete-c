#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/* #DEFINE'S -----------------------------------------------------------------*/
#define SDELIM "==STAGE %d============================\n" // stage delimiter
#define MDELIM "-------------------------------------\n"  // delimiter of -'s
#define THEEND "==THE END============================\n"   // end message
#define NOSFMT "Number of statements: %d\n"               // no. of statements
#define NOCFMT "Number of characters: %d\n"               // no. of chars
#define NPSFMT "Number of states: %d\n"                   // no. of states
#define TFQFMT "Total frequency: %d\n"                    // total frequency
#define ODNFMT "%d\n"                                     // one number format
#define NULLBT '\0'     // NULL byte
#define PMTNRD -1       // prompt NOT recognized code
#define PMTETY 0        // empty prompt code
#define PMTPCD 1        // successful prompt code
#define NLINEC '\n'     // newline character
#define CRTRNC '\r'     // carriage return character
#define DOTCHR '.'      // dot character
#define MAXLEN 37       // maximum output line length
#define NUMSEP 3        // number of dots in an ellipsis mark
#define VSTDNO 0        // not visited state status
#define VSTDYS 1        // visited state status
#define CMPSNO 0        // no compression requested status
#define CMPSYS 1        // compression requested status
#define SUCCESS 1       // success code
#define FAILURE 0       // failure code

/* TYPE DEFINITIONS ----------------------------------------------------------*/
typedef struct state state_t;   // a state in an automaton
typedef struct node  node_t;    // a node in a linked list

struct node {           // a node in a linked list of transitions has
    char*    str;       // ... a transition string
    state_t* state;     // ... the state reached via the string, and
    node_t*  next;      // ... a link to the next node in the list.
};

typedef struct {        // a linked list consists of
    node_t* head;       // ... a pointer to the first node and
    node_t* tail;       // ... a pointer to the last node in the list.
} list_t;

struct state {              // a state in an automaton is characterized by
    unsigned int id;        // ... an identifier,
    unsigned int freq;      // ... frequency of traversal,
    int          visited;   // ... visited status flag, and
    list_t*      outputs;   // ... a list of output states.
};

typedef struct {            // an automaton consists of
    state_t*     ini;       // ... the initial state, and
    unsigned int nid;       // ... the identifier of the next new state.
} automaton_t;

/* FUNCTION PROTOTYPES -------------------------------------------------------*/
/* FUNCTIONS FOR WORKING WITH LINKED LISTS -----------------------------------*/
list_t* makeemptylist(void);                            // make an empty list
void    freelist(list_t*);                              // free list

/* FUNCTIONS FOR WORKING WITH AUTOMATA ---------------------------------------*/
automaton_t* makeemptyautomaton(void);                  // make an empty automaton
state_t*     makenewstate(automaton_t*);                // make a new state
automaton_t* getfptautoamon(int*, int*);                // get automaton from stdin
int          addstatement(automaton_t*);                // add a statement to input
state_t*     getoutputstate(state_t*, char*);           // get output state
int          addtransition(state_t*, char*, state_t*);  // add transition
void         assignnode(node_t*, char*, state_t*, node_t*); // assign elements
void         processprompts(automaton_t*);              // process all input prompts
int          processprompt(automaton_t*);               // process one prompt
char*        generate(state_t*, char*);                 // generate characters
int          compressautomaton(automaton_t*, int);      // compress automaton
int          dfscompress(state_t*, int, int);           // attempt one compression
int          statecompress(state_t*, char*, state_t*);  // compress state
int          countfreq(automaton_t*);                   // count state frequencies
int          deletetransition(state_t*, char*);         // delete transition
void         freeautomaton(automaton_t*);               // free automaton
void         indexstates(state_t**, unsigned int);      // index states

/* USEFUL FUNCTIONS ----------------------------------------------------------*/
int mygetchar(void);                        // getchar() that skips
                                            // carriage returns
int prefix(const char*, const char*);       // check if one string is a
                                            // prefix of the other

/* WHERE IT ALL HAPPENS ------------------------------------------------------*/
int main(int argc, char *argv[]) {
    int stage = 0, ns, nc, n;

    printf(SDELIM, stage++);
    automaton_t* A = getfptautoamon(&ns, &nc);  // read an automaton from stdin
    printf(NOSFMT, ns);                         // print number of statements
    printf(NOCFMT, nc);                         // print number of characters
    printf(NPSFMT, A->nid);                     // print number of states

    printf(SDELIM, stage++);                    // print Stage 1 delimiter
    processprompts(A);                          // process prompts from stdin

    printf(SDELIM, stage);                      // print Stage 2 delimiter
    scanf(ODNFMT, &n);                          // read number of compressions
    int c = compressautomaton(A, n);            // perform n compressions in A
    printf(NPSFMT, A->nid - c);                 // print number of states
    printf(TFQFMT, countfreq(A));               // print total frequency
    printf(MDELIM);                             // print delimiter
    processprompts(A);                          // process prompts from stdin

    freeautomaton(A);                           // free automaton
    printf(THEEND);                             // print "THE END" message
    return EXIT_SUCCESS;                        // algorithms are fun!!!
}

/* FUNCTIONS FOR WORKING WITH LINKED LISTS -----------------------------------*/

// Create and return a new linked list with no nodes.
list_t* makeemptylist(void) {
    list_t* list = (list_t*)calloc(1, sizeof(list_t));  // zero-initialized
    assert(list != NULL);                               // head, tail = NULL
    return list;
}

// Release every node (and its label) in the list, then the list itself.
void freelist(list_t* list) {
    assert(list != NULL);
    for (node_t *curr = list->head, *next; curr != NULL; curr = next) {
        next = curr->next;      // save link before freeing
        free(curr->str);
        free(curr);
    }
    free(list);
}

/* FUNCTIONS FOR WORKING WITH AUTOMATA ---------------------------------------*/

// Make an empty automaton.
automaton_t* makeemptyautomaton(void) {
    automaton_t* automaton = (automaton_t*)malloc(sizeof(*automaton));
    assert(automaton != NULL);                  // check allocated memory
    automaton->nid = 0;                         // initialize state counter
    automaton->ini = makenewstate(automaton);   // create initial state
    return automaton;                           // return new empty automaton
}

// Make a new state for the given automaton.
state_t* makenewstate(automaton_t* automaton) {
    assert(automaton != NULL);
    state_t* state = (state_t*)malloc(sizeof(*state));
    assert(state != NULL);                      // check allocated memory
    state->id      = automaton->nid++;          // set state identifier
    state->freq    = 0;                         // initialize visit frequency
    state->visited = VSTDNO;                    // set status to not visited
    state->outputs = makeemptylist();           // initialize list of outputs
    return state;                               // return new state
}

// Extract a frequency prefix automaton from stdin. Store the number of stateme-
// nts and characters used to extract the automaton in ns and nc, respectively.
automaton_t* getfptautoamon(int* ns, int* nc) {
    assert(ns != NULL && nc != NULL);           // check inputs
    automaton_t* automaton = makeemptyautomaton();
    *ns = *nc = 0;                              // initialize counters to zeros
    int nchars;
    while ((nchars = addstatement(automaton))) {    // while more chars added
        (*ns)++;                                // increment statement counter
        (*nc) += nchars;                        // increment character counter
    }
    return automaton;                           // return extracted automaton
}

// Extract a statement from stdin, add it to the given automaton, and return the
// number of characters in the extracted statement.
// NB. Assumes all the state transitions have single character labels.
int addstatement(automaton_t* automaton) {
    assert(automaton != NULL);                  // check input automaton
    int result = 0;                             // initialize the result to zero
    char str[2] = {NULLBT, NULLBT};             // next character buffer
    state_t* curr = automaton->ini;             // initialize current state
    // while the next character read from stdin is not EOF or '\n' character
    while ((str[0] = mygetchar()) != EOF && str[0] != NLINEC) {
        result++;                   // increment number of processed characters
        curr->freq++;               // increment current state traversal frequency
        // get output state of the current state reachable via str
        state_t* state = getoutputstate(curr, str);
        if (state == NULL) {        // if such output state does not exist
            // make a new state to become an output of the curr state
            state_t* output = makenewstate(automaton);
            char* via = (char*)malloc(2 * sizeof(char));
            assert(via != NULL);    // check allocated memory
            strcpy(via, str);       // create transition label
            // add a transition from curr state via 'via' string to output state
            addtransition(curr, via, output);
            curr = output;          // set the new state to be the curr state
        } else {                    // if such output state exists
            curr = state;           // ... set it as current state
        }
    }
    return result;  // return the number of characters in the statement
}

// Get the output state of a given state reachable via the input string.
state_t* getoutputstate(state_t* state, char* str) {
    assert(state != NULL && str != NULL);
    node_t* curr = state->outputs->head;
    while (curr) {                          // while more outputs available
        if (!strcmp(curr->str, str)) {      // if requested output found
            return curr->state;             // ... return it
        }
        curr = curr->next;                  // move to the next output state
    }
    return NULL;                            // no requested output found
}

// Add transition from the 'from' state to the 'to' state via the 'via' string.
// Return SUCCESS if the transition was added; otherwise, return FAILURE.
// NB. Assumes the nodes in the from->outputs list are in ascending ASCIIbetical
//     order by transition strings at the time of a function call, and ensures
//     the order is preserved after the function call completes.
int addtransition(state_t* from, char* via, state_t* to) {
    assert(from != NULL && via != NULL && to != NULL);  // check inputs
    node_t* curr = from->outputs->head;     // get head of the outputs list
    node_t* node = (node_t*)malloc(sizeof(*node));
    assert(node != NULL);                   // check allocated memory
    if (curr == NULL) {                     // if there are no output states
        // set node to represent the 'via' transition to state 'to'
        assignnode(node, via, to, NULL);
        // as from has only one output, point head and tail of the list to it
        from->outputs->head = from->outputs->tail = node;
        return SUCCESS;
    }
    while (curr) {                          // iterate over outputs of state from
        int cmpres = strcmp(curr->str, via);
        if (cmpres == 0) {      // if via is the label to the current output state
            free(node);         // free allocated memory
            return FAILURE;     // no need to model this transition again
        } else if (cmpres > 0) {    // if curr->str is greater (in ASCII) than via
            // ... insert transition before the curr transition in the list
            // make new list node a copy of the curr node
            assignnode(node, curr->str, curr->state, curr->next);
            // use the curr node to represent the requested transition
            assignnode(curr, via, to, node);
            if (node->next == NULL) {       // if node is last in the list
                from->outputs->tail = node; // ... point list tail to it
            }
            return SUCCESS;
        }
        curr = curr->next;      // move to the next output state in the list
    }
    // if all outputs were traversed and no transition inserted, insert at tail
    assignnode(node, via, to, NULL);        // create list node
    from->outputs->tail->next = node;       // insert transition at tail
    from->outputs->tail = node;             // reset tail to point to node
    return SUCCESS;
}

// Assign elements of the given node in the list of output states.
void assignnode(node_t* node, char* str, state_t* state, node_t* next) {
    assert(node != NULL && str != NULL && state != NULL);
    node->str   = str;          // assign transition string
    node->state = state;        // assign output state
    node->next  = next;         // assign next node in the list
}

// Process all input prompts.
void processprompts(automaton_t* automaton) {
    assert(automaton != NULL);                  // check the input automaton
    while (processprompt(automaton) != PMTETY) {    // if the prompt is not empty
        putchar(NLINEC);            // end processing of the current prompt
    }
}

// Process prompt from the standard input by matching it in the automaton and
// generating its continuation by extracting the most likely next characters.
// The function returns PMTNRD if the prompt is NOT recognized, PMTETY if the
// input prompt is empty, and PMTPCD if the prompt was processed successfully.
int processprompt(automaton_t* automaton) {
    assert(automaton != NULL);                  // check the input automaton
    int c, count = 0, pos = 0, matched = 1;
    char trn[MAXLEN + 1];                       // transition string storage
    state_t *curr = automaton->ini, *next = NULL;

    // process the input prompt character by character
    while ((c = mygetchar()) != EOF && c != NLINEC) {
        if (count == MAXLEN || !matched) {
            continue;                           // skip prompt characters
        }
        putchar(c);                             // print prompt character to stdout
        count++;                                // one more character printed
        trn[pos] = (char)c;                     // store printed character
        trn[pos + 1] = NULLBT;                  // terminate transition string
        next = getoutputstate(curr, trn);       // get output reachable via trn
        if (next != NULL) {                     // if such output exists
            curr = next;                        // ... move to it
            pos = 0;                            // restart transition string
            trn[0] = NULLBT;                    // terminate string properly
        } else {
            pos++;                              // keep building transition string
            node_t* cn = curr->outputs->head;   // get access to the first output
            int isprefix = 0;
            while (cn) {                        // next output states available
                isprefix |= prefix(trn, cn->str);
                if (isprefix) {                 // if transition string is a prefix
                    break;                      // ... stop checking outputs
                }
                cn = cn->next;
            }
            if (!isprefix) {
                matched = 0;                    // input prompt not in automaton
            }
        }
    }

    // check if it is necessary to generate a continuation of the prompt
    if (count == MAXLEN) {  // prompt is longer than allowed generated statement
        return PMTPCD;      // prompt processed successfully
    } else if (count == 0) {
        return PMTETY;      // prompt is empty
    }

    int dotc = 0;           // dots counter in the ellipsis mark
    while (++dotc <= NUMSEP && ++count <= MAXLEN) {
        putchar(DOTCHR);    // print a dot character to stdout
    }

    // generate continuation of the prompt
    char* gen;              // string to store generated characters
    // while more characters generated that fit maximum requested output length
    while ((gen = generate(curr, trn)) && count < MAXLEN) {
        int i = 0;
        while (gen[pos + i] && ++count <= MAXLEN) { // while still space available
            putchar(gen[pos + i]);                  // print character to stdout
            i++;
        }
        curr = getoutputstate(curr, gen);   // move to next state
        trn[0] = '\0';                      // cleanup transition string storage
        pos = 0;                            // reset position
    }
    return PMTPCD;                          // prompt successfully processed
}

// Get the transition string that starts with the given prefix string toward the
// most frequent output state of the given state.
char* generate(state_t* state, char* pfx) {
    assert(state != NULL && pfx != NULL);   // check inputs
    node_t* curr = state->outputs->head;    // head of output states list
    int maxfreq = 0;                        // maximum frequency
    char* result = NULL;                    // initialize result
    while (curr) {                          // while output states available
        // if pfx is a prefix of the transition string to the current output
        if (prefix(pfx, curr->str) && curr->state->freq >= maxfreq) {
            maxfreq = curr->state->freq;    // update maximum frequency
            result = curr->str;             // set result
        }
        curr = curr->next;                  // move to next output state
    }
    return result;
}

// Perform up to n compressions in the given automaton. Return the number of
// successful compressions.
int compressautomaton(automaton_t* automaton, int n) {
    assert(automaton != NULL);              // check the input automaton
    int result = 0;
    // while less than requested compressions and the next one is successful
    while (result < n && dfscompress(automaton->ini, VSTDYS, CMPSYS)) {
        result++;                           // count successful compressions
        // cleanup visited statuses left after the previous compression
        dfscompress(automaton->ini, VSTDNO, CMPSNO);
    }
    return result;
}

// Traverses the arcs of the automaton by performing the depth-first search
// (dfs), starting from state 'state'. Sets the visited status of all the
// visited states to 'visited'. If compression requested ('compress' is NOT set
// to CMPSNO), implements compression of the automaton defined by the first
// valid encountered arc. Returns the number of performed compressions.
int dfscompress(state_t* state, int visited, int compress) {
    assert(state != NULL);                  // check the input state
    state->visited = visited;               // set visited status
    node_t* output = state->outputs->head;  // get first output state
    while (output) {                        // while output states available
        // if compression requested and compression successful
        if (compress != CMPSNO &&
            statecompress(state, output->str, output->state)) {
            return 1;                       // one state compressed
        }
        if (output->state->visited != visited) {    // if more states available
            if (dfscompress(output->state, visited, compress)) {
                return 1;
            }
        }
        output = output->next;              // move to next output state
    }
    return 0;                               // no states were compressed
}

// Check compression conditions defined by an arc from state 'from' to state
// 'to' via 'str' string and, if satisfied, implement the compression.
// Returns SUCCESS if the compression successful; otherwise, returns FAILURE.
int statecompress(state_t* from, char* str, state_t* to) {
    node_t* i = from->outputs->head;        // first output state of 'from' state
    node_t* j = to->outputs->head;          // first output state of 'to' state
    if ((i != NULL && i->next != NULL) || j == NULL) {
        // 'from' has more than one outgoing arc OR 'to' has no outgoing arcs
        return FAILURE;
    }
    // conditions for compression satisfied
    while (j) {                     // while output states of 'to' state available
        // construct fresh transition arc label
        char* t = (char*)malloc((strlen(str) + strlen(j->str) + 1) * sizeof(char));
        strcpy(t, str);
        strcat(t, j->str);
        addtransition(from, t, j->state);   // add fresh automaton arc
        j = j->next;                        // move to next output state of 'to'
    }
    freelist(to->outputs);                  // free old output states of 'to'
    deletetransition(from, str);
    return SUCCESS;
}

// Count traversal frequencies of all the states of the given automaton.
int countfreq(automaton_t* automaton) {
    assert(automaton != NULL);              // check input automaton
    int result = 0;                         // initialize result to zero
    state_t** states = (state_t**)calloc(automaton->nid, sizeof(state_t*));
    states[0] = automaton->ini;             // index the initial state
    indexstates(states, 0);                 // index states starting from the
                                            // ... initial state
    for (int i = 0; i < automaton->nid; i++) {  // iterate all used state ids
        if (states[i] != NULL) {            // if state with id i exists
            result += states[i]->freq;      // count state traversal frequencies
        }
    }
    free(states);                           // free temporary array of states
    return result;
}

// Delete transition defined by the input string. Return 1 if the transition
// was deleted successfully; otherwise return 0.
int deletetransition(state_t* state, char* str) {
    assert(state != NULL && str != NULL);   // check input list and string
    node_t *curr = state->outputs->head, *prev = NULL;
    while (curr) {                          // iterate over the list of outputs
        if (strcmp(curr->str, str) == 0) {  // if the transition was found ...
            if (prev == NULL) {             // ... head of the list defines it
                state->outputs->head = curr->next;  // the second node is head
                if (state->outputs->head == NULL) {
                    state->outputs->tail = NULL;
                }
            } else {                        // if transition is not in the head
                if (curr->next == NULL) {   // ... but in the tail of the list
                    state->outputs->tail = prev;    // set tail to point to prev
                    prev->next = NULL;      // prev is now the last node
                } else {
                    prev->next = curr->next;
                }
            }
            free(curr->str);                // free the label of the arc
            free(curr->state);              // free the output state
            free(curr);                     // free the node of the transition
            return 1;                       // transition was removed
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;                               // transition did not exist
}

// Free the input automaton.
void freeautomaton(automaton_t* automaton) {
    assert(automaton != NULL);              // check automaton exists
    state_t** states = (state_t**)calloc(automaton->nid, sizeof(state_t*));
    states[0] = automaton->ini;             // index the initial state
    indexstates(states, 0);                 // index states
    for (int i = 0; i < automaton->nid; i++) {  // iterate all used state ids
        if (states[i] != NULL) {            // if state with ID i exists
            freelist(states[i]->outputs);   // free outputs of state with ID i
            free(states[i]);                // free state
        }
    }
    free(states);                           // free temporary array of states
    free(automaton);                        // free automaton memory
}

// Index states starting from state p.
void indexstates(state_t** states, unsigned int p) {
    assert(states != NULL && states[p] != NULL);    // check input
    node_t* output = states[p]->outputs->head;
    while (output) {                                // iterate outputs of states[p]
        if (states[output->state->id] == NULL) {    // if output state not indexed
            states[output->state->id] = output->state;  // ... index it
            indexstates(states, output->state->id);
        }
        output = output->next;                      // move to next output
    }
}

/* USEFUL FUNCTIONS ----------------------------------------------------------*/

// Read the next character from stdin, ignoring any '\r' characters.
int mygetchar(void) {
    int c;
    do {
        c = getchar();
    } while (c == CRTRNC);
    return c;
}

// Check if string pfx is a prefix of string str.
int prefix(const char* pfx, const char* str) {
    int pos = 0;                            // position in the prefix string
    while (pfx[pos]) {                      // while prefix characters available
        if (pfx[pos] != str[pos]) {         // compare pfx and str characters at pos
            return FAILURE;                 // pfx is not a prefix of str
        }
        pos++;                              // move to the next position in pfx
    }
    return SUCCESS;                         // pfx is a prefix of str
}
