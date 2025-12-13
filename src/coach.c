#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include "coach.h"

// Fonction pour supprimer les espaces/tabs à la fin d'une chaîne
void trim_trailing_whitespace(char *str) {
    if (str == NULL) return;
    
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' || str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
        len--;
    }
}

// Fonction pour supprimer les espaces/tabs au début et à la fin
void trim_whitespace(char *str) {
    if (str == NULL) return;
    
    // Trim leading
    char *start = str;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
    }
    
    // Déplacer si nécessaire
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    // Trim trailing
    trim_trailing_whitespace(str);
}

// ----------------------------------------------------
//               ADD COACH - MODIFIÉ (10 CHAMPS)
// ----------------------------------------------------
int addCoach(const char *filename, Coach c)
{
    FILE *f = fopen(filename, "a");
    if (!f)
    {
        printf("ERROR: Cannot open file %s for writing\n", filename);
        return 0;
    }

    char gender_str[10];
    strcpy(gender_str, (c.gender == 1) ? "Male" : "Female");

    // Écriture avec 10 champs
    int result = fprintf(f, "%d %s %s %d %d %d %s %s %s %s\n",
            c.id, c.lastName, c.firstName,
            c.dateOfBirth.day, c.dateOfBirth.month, c.dateOfBirth.year,
            c.center, c.phoneNumber, gender_str, c.specialty);
    
    fclose(f);
    
    if (result < 0) {
        printf("ERROR: Failed to write to file %s\n", filename);
        return 0;
    }
    
    printf("SUCCESS: Coach added to %s - ID: %d, Name: %s %s, Specialty: %s\n", 
           filename, c.id, c.firstName, c.lastName, c.specialty);
    return 1;
}

// ----------------------------------------------------
//               MODIFY COACH (by ID) - MODIFIÉ
// ----------------------------------------------------
int modifyCoach(const char *filename, int id, Coach updated)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        printf("ERROR: Cannot open file %s for reading\n", filename);
        return 0;
    }
    
    FILE *f2 = fopen("temp.txt", "w");
    if (!f2)
    {
        fclose(f);
        printf("ERROR: Cannot create temp file\n");
        return 0;
    }

    Coach c;
    int found = 0;
    char gender_str[10];
    char specialty[30];
    char updated_gender_str[10];
    
    strcpy(updated_gender_str, (updated.gender == 1) ? "Male" : "Female");

    // Lire 10 champs
    while (fscanf(f, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                  &c.id, c.lastName, c.firstName,
                  &c.dateOfBirth.day, &c.dateOfBirth.month, &c.dateOfBirth.year,
                  c.center, c.phoneNumber, gender_str, specialty) == 10)
    {
        // Nettoyer les chaînes
        trim_trailing_whitespace(c.center);
        trim_trailing_whitespace(specialty);
        
        if (c.id == id)
        {
            // Écrire 10 champs modifiés
            fprintf(f2, "%d %s %s %d %d %d %s %s %s %s\n",
                    updated.id, updated.lastName, updated.firstName,
                    updated.dateOfBirth.day, updated.dateOfBirth.month, updated.dateOfBirth.year,
                    updated.center, updated.phoneNumber, updated_gender_str, updated.specialty);
            found = 1;
            printf("SUCCESS: Coach ID %d modified in %s\n", id, filename);
        }
        else
        {
            // Réécrire les 10 champs originaux
            fprintf(f2, "%d %s %s %d %d %d %s %s %s %s\n",
                    c.id, c.lastName, c.firstName,
                    c.dateOfBirth.day, c.dateOfBirth.month, c.dateOfBirth.year,
                    c.center, c.phoneNumber, gender_str, specialty);
        }
    }

    fclose(f);
    fclose(f2);

    if (found)
    {
        remove(filename);
        rename("temp.txt", filename);
    }
    else
    {
        remove("temp.txt");
        printf("ERROR: Coach ID %d not found in %s\n", id, filename);
    }

    return found;
}

// ----------------------------------------------------
//               DELETE COACH (by ID) - MODIFIÉ
// ----------------------------------------------------
int deleteCoach(const char *filename, int id)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        printf("ERROR: Cannot open file %s for reading\n", filename);
        return 0;
    }
    
    FILE *f2 = fopen("temp.txt", "w");
    if (!f2)
    {
        fclose(f);
        printf("ERROR: Cannot create temp file\n");
        return 0;
    }

    Coach c;
    int found = 0;
    char gender_str[10];
    char specialty[30];

    // Lire 10 champs
    while (fscanf(f, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                  &c.id, c.lastName, c.firstName,
                  &c.dateOfBirth.day, &c.dateOfBirth.month, &c.dateOfBirth.year,
                  c.center, c.phoneNumber, gender_str, specialty) == 10)
    {
        if (c.id == id)
        {
            found = 1;
            printf("SUCCESS: Coach ID %d deleted from %s\n", id, filename);
        }
        else
        {
            // Écrire 10 champs (sauf celui supprimé)
            fprintf(f2, "%d %s %s %d %d %d %s %s %s %s\n",
                    c.id, c.lastName, c.firstName,
                    c.dateOfBirth.day, c.dateOfBirth.month, c.dateOfBirth.year,
                    c.center, c.phoneNumber, gender_str, specialty);
        }
    }

    fclose(f);
    fclose(f2);

    if (found)
    {
        remove(filename);
        rename("temp.txt", filename);
    }
    else
    {
        remove("temp.txt");
        printf("ERROR: Coach ID %d not found in %s\n", id, filename);
    }

    return found;
}

// ----------------------------------------------------
//               SEARCH COACH - MODIFIÉ (10 CHAMPS)
// ----------------------------------------------------
Coach searchCoach(const char *filename, const char *lastName, const char *center)
{
    FILE *f = fopen(filename, "r");
    Coach c;
    c.id = -1; // Initialiser avec ID -1 pour indiquer "non trouvé"
    char gender_str[10];
    char specialty[30];
    
    if (!f)
    {
        printf("ERROR: Cannot open file %s for reading\n", filename);
        return c;
    }

    // Lire 10 champs
    while (fscanf(f, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                  &c.id, c.lastName, c.firstName,
                  &c.dateOfBirth.day, &c.dateOfBirth.month, &c.dateOfBirth.year,
                  c.center, c.phoneNumber, gender_str, specialty) == 10)
    {
        // Convertir gender string en int
        c.gender = (strcmp(gender_str, "Male") == 0) ? 1 : 2;
        
        // Stocker la spécialité
        strncpy(c.specialty, specialty, sizeof(c.specialty)-1);
        c.specialty[sizeof(c.specialty)-1] = '\0';
        
        // Nettoyer les chaînes
        trim_trailing_whitespace(c.center);
        trim_trailing_whitespace(c.specialty);
        
        // Préparer les chaînes de recherche
        char search_lastName[30] = "";
        char search_center[30] = "";
        
        if (lastName && strlen(lastName) > 0) {
            strncpy(search_lastName, lastName, sizeof(search_lastName)-1);
            search_lastName[sizeof(search_lastName)-1] = '\0';
            trim_whitespace(search_lastName);
        }
        
        if (center && strlen(center) > 0) {
            strncpy(search_center, center, sizeof(search_center)-1);
            search_center[sizeof(search_center)-1] = '\0';
            trim_whitespace(search_center);
        }
        
        // Comparaison (insensible à la casse)
        int matchLastName = 1;
        int matchCenter = 1;
        
        if (strlen(search_lastName) > 0) {
            matchLastName = (strcasecmp(c.lastName, search_lastName) == 0);
        }
        
        if (strlen(search_center) > 0) {
            matchCenter = (strcasecmp(c.center, search_center) == 0);
        }

        if (matchLastName && matchCenter)
        {
            fclose(f);
            printf("SUCCESS: Coach found in %s - ID: %d, Name: %s %s, Specialty: %s\n", 
                   filename, c.id, c.firstName, c.lastName, c.specialty);
            return c;
        }
    }

    fclose(f);
    c.id = -1; // Not found
    printf("INFO: Coach not found in %s with criteria: lastName='%s', center='%s'\n", 
           filename, lastName ? lastName : "(any)", center ? center : "(any)");
    return c;
}
