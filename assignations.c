//
// Created by Gilles Sérasset on 09/10/2019.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "tokens.h"
#include "lookahead_lexer.h"
#include "uthash.h"

#include "assignations.h"


/*********** Managing Symbol Table with UTHash Hash table ***************/

struct Symbol {
    float value;
    char var[100];     /* we'll use this field as the key */
    UT_hash_handle hh; /* makes this structure hashable */
};

struct Symbol *symbols = NULL;

void set_value(char var[], float val) {
    struct Symbol* symbol;
    HASH_FIND_STR( symbols, var, symbol );
    if (symbol == NULL) {
        // the symbol does not exist yet
        symbol = calloc(1, sizeof(struct Symbol));
        strcpy(symbol->var, var);
        HASH_ADD_STR( symbols, var, symbol );
    }
    symbol->value = val;
}

float get_value(char var[]) {
    struct Symbol* symbol;
    HASH_FIND_STR( symbols, var, symbol );
    if (symbol == NULL) {
        return 0.;
    } else {
        return symbol->value;
    }
}

void print_symbols() {
    struct Symbol *s;

    for(s=symbols; s != NULL; s=s->hh.next) {
        fprintf(stdout, "%s = %f\n", s->var, s->value);
    }
}



typedef struct arbre_t *arbre_tt;
struct arbre_t {
    enum yytokentype type;
    struct arbre_t *fg, *fd;
    char * nom;
};

arbre_tt AA=NULL;

typedef struct list_tt *list_t;
struct list_tt {
    arbre_tt adresse;
    list_t suivant;
};

list_t garbageCollector = NULL;

void ajouter (arbre_tt arbre1){
    list_t tmp = malloc(sizeof(struct list_tt));
    tmp->suivant = garbageCollector;
    tmp->adresse = arbre1;
    garbageCollector = tmp;
}
void supprimer (int a){
    list_t tmp1 = garbageCollector;
    list_t tmp2;
    while(!tmp1){
        tmp2=tmp1->suivant;
        if (a){
            free (tmp1->adresse);
        }
        free(tmp1);
        tmp1=tmp2;
    }
    garbageCollector=NULL;
}

void erreur () {
    //perror("!!! ERREUR !!!\n");
    fprintf(stdout, "!!! ERREUR !!!\n");
    supprimer(1);
    exit(1);
}

float miseAJour(arbre_tt ArA){
    arbre_tt tmp = ArA;
    arbre_tt fg;
    arbre_tt fd;
    float valeur, valeur1, valeur2;
    if (tmp != NULL){
        fg=tmp->fg;
        fd=tmp->fd;
        switch (tmp->type){
            case ASSIGN:
                valeur = miseAJour(fd);
                set_value(fg->nom, valeur);
                break;
            case PLUS : 
                valeur1 = miseAJour(fg);
                valeur2 = miseAJour(fd);
                valeur= valeur1 + valeur2;
                break;
            case MINUS : 
                valeur1 = miseAJour(fg);
                valeur2 = miseAJour(fd);
                valeur= valeur1 - valeur2;
                break;
            case MULT : 
                valeur1 = miseAJour(fg);
                valeur2 = miseAJour(fd);
                valeur= valeur1 * valeur2;
                break;
                
            case DIV : 
                valeur1 = miseAJour(fg);
                valeur2 = miseAJour(fd);
                valeur= valeur1 / valeur2;
                break;
            case EXPON : 
                valeur1 = miseAJour(fg);
                valeur2 = miseAJour(fd);
                valeur= powf(valeur1, valeur2);
                break;
            case NUMBER :
                valeur = atof(tmp->nom);
                break;
            case VAR:
                valeur=get_value(tmp->nom);
                break;
            case PI:
                valeur = 0.0;
                valeur1 = miseAJour(fg);
                valeur2 = miseAJour(fd);
                break;
            case ABS:
                valeur = miseAJour(fg);
                if (valeur < 0){
                    valeur = -valeur;
                }
                break;
        }
        return valeur;
    }
    return 0.0;
}

void my_free(arbre_tt ArA){
    arbre_tt tmp = ArA;
    arbre_tt fg;
    arbre_tt fd;
    if (tmp!=NULL){
        fg=tmp->fg;
        fd=tmp->fd;
        free(tmp);
        my_free(fg);
        my_free(fd);
        
    }

}

/************** Grammaire *****************/
arbre_tt A();
arbre_tt T_prime(arbre_tt graine) {
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case EXPON :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=EXPON;
                tmp->fg= graine;
                tmp->fd = NULL;
                next();
                
                ajouter(tmp);
                
                if (currentToken()==NULL ){
                    erreur();
                }
                if (currentToken()->type!= LB) {
                    erreur();
                }
                
                next();
                tmp2=A();
                if (tmp2 != NULL){
                    tmp->fd=tmp2;
                }
                
                if (currentToken()==NULL || currentToken()->type!= RB){
                    erreur();
                }
                
                next();
                tmp2=T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case DIV:
            case MULT:
            case MINUS:
            case PLUS:
            case SEMICOLON:
            case RB:
            case ABS:
                break;
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}



arbre_tt T(){
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case NUMBER :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=NUMBER;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case VAR:
                tmp = malloc(sizeof(struct arbre_t));
                
                tmp->type=VAR;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case LB : 
                next();
                tmp = A();
                if (!currentToken() || currentToken()->type!= RB) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
             case ABS : 
                next();
                tmp = A();
                if (!currentToken() || currentToken()->type!= ABS) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2 = malloc(sizeof(struct arbre_t));
                tmp2->type = ABS;
                tmp2->fg = tmp;
                tmp2->fd = NULL;
                tmp = tmp2;
                ajouter(tmp);
                
                break;      
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}


arbre_tt D_prime(arbre_tt graine) {
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case DIV :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=DIV;
                tmp->fg= graine;
                tmp->fd = NULL;
    
                ajouter(tmp);
    
                next();
                tmp2=T();
                if (tmp2 != NULL){
                    tmp->fd=tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case MULT:
            case MINUS:
            case PLUS:
            case RB:
            case SEMICOLON:
            case ABS:
                break;
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}


arbre_tt D(){
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case NUMBER :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=NUMBER;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case VAR:
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=VAR;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case LB : 
                next();
                tmp = A();
                
                if (currentToken()==NULL || currentToken()->type!= RB) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
              
            case ABS : 
                next();
                tmp = A();
                
                if (currentToken()==NULL || currentToken()->type!= ABS) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                
                tmp2 = malloc(sizeof(struct arbre_t));
                tmp2->type = ABS;
                tmp2->fg = tmp;
                tmp2->fd = NULL;
                tmp = tmp2;
                ajouter(tmp);
                
                break;
                 
               
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}


arbre_tt F_prime(arbre_tt graine) {
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case MULT :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=MULT;
                tmp->fg= graine;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2=D();
                if (tmp2 != NULL){
                    tmp->fd=tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case MINUS:
            case SEMICOLON:
            case PLUS:
            case RB:
            case ABS:
                break;
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}


arbre_tt F(){
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case NUMBER :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=NUMBER;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case VAR:
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=VAR;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
            
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case LB : 
                next();
                tmp = A();
                if (currentToken()==NULL || currentToken()->type!= RB) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;

            case ABS : 
                next();
                tmp = A();
                if (currentToken()==NULL || currentToken()->type!= ABS) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                
                tmp2 = malloc(sizeof(struct arbre_t));
                tmp2->type = ABS;
                tmp2->fg = tmp;
                tmp2->fd = NULL;
                tmp = tmp2;
                
                ajouter(tmp);
                
                break;

            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}


arbre_tt M_prime(arbre_tt graine) {
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        
        switch (currentToken()->type){
            case MINUS :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=MINUS;
                tmp->fg= graine;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2=F();
                if (tmp2 != NULL){
                    tmp->fd=tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case RB:
            case SEMICOLON:
            case PLUS:
            case ABS:
                break;
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}

arbre_tt M(){
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case NUMBER :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=NUMBER;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case VAR:
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=VAR;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case LB : 
                next();
                tmp = A();
                if (currentToken()==NULL || currentToken()->type!= RB) {
                    erreur();
                    
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;

            case ABS : 
                next();
                tmp = A();
                if (currentToken()==NULL || currentToken()->type!= ABS) {
                    erreur();
                    
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                
                tmp2 = malloc(sizeof(struct arbre_t));
                tmp2->type = ABS;
                tmp2->fg = tmp;
                tmp2->fd = NULL;
                tmp = tmp2;
                ajouter(tmp);
                
                break;


            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}


arbre_tt P_prime(arbre_tt graine) {
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case PLUS :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=PLUS;
                tmp->fg= graine;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2=M();
                if (tmp2 != NULL){
                    tmp->fd=tmp2;
                }
                tmp2=P_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case RB:
            case SEMICOLON:
            case ABS:
                break;
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}

arbre_tt P(){
    if (currentToken() != NULL ) {
        arbre_tt tmp = NULL;
        arbre_tt tmp2;
        switch (currentToken()->type){
            case NUMBER :
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=NUMBER;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=P_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case VAR:
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=VAR;
                tmp->nom = get_text(currentToken());
                tmp->fg= NULL;
                tmp->fd = NULL;
                
                ajouter(tmp);
                            
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=P_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
            case LB : 
                next();
                tmp = A();
                if (currentToken()==NULL || currentToken()->type!= RB) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=P_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                break;
                
            case ABS : 
                next();
                tmp = A();
                if (currentToken()==NULL || currentToken()->type!= ABS) {
                    erreur();
                }
                next();
                tmp2 = T_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=D_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=F_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=M_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                tmp2=P_prime(tmp);
                if (tmp2 != NULL){
                    tmp = tmp2;
                }
                
                tmp2 = malloc(sizeof(struct arbre_t));
                tmp2->type = ABS;
                tmp2->fg = tmp;
                tmp2->fd = NULL;
                tmp = tmp2;
                ajouter(tmp);
                
                break;
                
                
            default :
                erreur();
        }
        return tmp;
    }
    return NULL;
}

arbre_tt A(){
    if (currentToken()!= NULL){
        arbre_tt tmp;
        switch (currentToken()->type){
            case VAR :
                switch (lookup(2)->type){
                    case ASSIGN :
                        tmp = malloc (sizeof(struct arbre_t));
                        tmp->type = ASSIGN;
                        ajouter(tmp);
                        tmp->fg = malloc (sizeof(struct arbre_t));
                        ajouter(tmp->fg);
                        arbre_tt arbre_g = tmp->fg;
                        arbre_g->type = VAR;
                        arbre_g->fd = NULL;
                        arbre_g->fg = NULL;
                        arbre_g->nom = get_text(currentToken());
                        
                        next();
                        next();
                        tmp->fd=A(); 
                        return tmp;
                        break;
                    
                    default : return P();
                }
                
                break;
            case NUMBER:
            case LB : 
                return P();
                break;
                
            case ABS : 
                next();
                tmp = malloc(sizeof(struct arbre_t));
                tmp->type=ABS;//pour coder la valeur absolue
                ajouter(tmp);
                
                tmp->fg= A();
                tmp->fd = NULL;
                if (currentToken() == NULL || currentToken()->type!= ABS) {
                    erreur();
                    
                }
                next();
                return tmp;
                break;
            
            default : //printf("%d\n", currentToken()->type);
                    erreur();
        }
            
        
        
    }
}
void S(){
    if (currentToken()!= NULL){
        arbre_tt tmp, tmp2;
        switch (currentToken()->type){
            case VAR:
            case NUMBER:
            case ABS :
            case LB :
                tmp = A();
                
                if (currentToken()==NULL || currentToken()->type != SEMICOLON){
                    erreur();
                }
                next();
                if (!AA){
                    AA=tmp;
                }else{
                    tmp2=malloc(sizeof(struct arbre_t));
                    tmp2->type=PI;
                    tmp2->fg = AA;
                    tmp2->fd = tmp;
                    AA = tmp2;
                }
                if (currentToken()) {
                    S();
                }
                    
                break;
            default : 
                erreur();
        }
    }
}




/*********** PROGRAMME PRINCIPAL ************/
int main(int argc, char *argv[]) {
    // fprintf(stderr, "Lexing from %s.\n", argv[1]);
    initLexer(argv[1]);
    // APPEL A LA SOURCE DE LA GRAMMAIRE
    S();
    
    // ON VERIFIE QUE LA GRAMMAIRE A BIEN TERMINE SON TRAVAIL A LA FIN DU MOT A ANALYSER
    if (currentToken() != NULL) {
        fprintf(stderr, "Unexpected input after assignation.\n");
        fprintf(stdout, "!!! ERREUR !!!\n");
        //LIBERATION DE LA MEMOIRE
        supprimer(1);
        return 1;
    }
    //LIBERER LE GARBAGE COLLECTOR SEUL
    supprimer(0);
    
    //CALCULS DES EXPRESSIONS
    miseAJour(AA);
    
    // ET ON AFFICHE LA TABLE DES SYMBOLES
    print_symbols();
    
    //LIBERATION DE LA MEMOIRE
    my_free(AA);
    return 0;
}

