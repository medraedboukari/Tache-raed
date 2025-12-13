#ifndef COACH_H_INCLUDED
#define COACH_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

// ----------------------------
//          STRUCTURES
// ----------------------------
typedef struct
{
    int day;
    int month;
    int year;
} Date;

typedef struct
{
    int id;
    char lastName[30];
    char firstName[30];
    Date dateOfBirth;
    char center[30];
    char phoneNumber[20];
    int gender; // 1 = Male, 2 = Female
    char specialty[30];
} Coach;

// Structure pour l'assignation coach-cours
typedef struct
{
    int coach_id;
    char coach_name[50];
    char coach_phone[20];
    char coach_specialty[30];  // AJOUTÉ
    char course_id[10];
    char course_name[50];
    char course_center[30];
    char course_time[20];
    int day;
    int month;
    int year;
} CoachAssignment;

// ----------------------------
//         PROTOTYPES
// ----------------------------

// Fonctions pour les coachs
int addCoach(const char *filename, Coach c);
int modifyCoach(const char *filename, int id, Coach updated);
int deleteCoach(const char *filename, int id);
Coach searchCoach(const char *filename, const char *lastName, const char *center);

// Fonctions pour les assignations aux cours
int assignCoachToCourse(const char *filename, CoachAssignment assignment);
int isCoachAssigned(const char *filename, int coach_id, const char *course_id);
int getCoachCountForCourse(const char *filename, const char *course_id, int capacity);
void loadCourseAssignments(GtkTreeView *treeview, const char *filename);

#endif // COACH_H_INCLUDED
