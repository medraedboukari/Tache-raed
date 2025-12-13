#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "coach.h"
#include <curl/curl.h>


// Fonction helper pour lire les données
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
    const char **data = (const char **)userp;
    size_t room = size * nmemb;
    
    if(!data || !*data || **data == '\0')
        return 0;
    
    size_t len = strlen(*data);
    if(room < len)
        len = room;
    
    memcpy(ptr, *data, len);
    *data += len;
    
    return len;
}

// =========================================================
// FONCTION DE DEBUG: Afficher le contenu du fichier coach.txt
// =========================================================
void debug_coach_file() {
    printf("\n=== DEBUG coach.txt ===\n");
    FILE *f = fopen("coach.txt", "r");
    if (!f) {
        printf("ERROR: Cannot open coach.txt (errno: %d)\n", errno);
        return;
    }
    
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        printf("Line %d: %s\n", line_num, line);
        
        // Essayer de parser pour vérifier le format
        int id, day, month, year;
        char lastName[30], firstName[30], center[30], phone[20], gender[10], specialty[30];
        
        int fields = sscanf(line, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                           &id, lastName, firstName, &day, &month, &year,
                           center, phone, gender, specialty);
        
        printf("  Parsed %d fields: ID=%d, Name=%s %s, Specialty=%s\n", 
               fields, id, firstName, lastName, specialty);
    }
    fclose(f);
    printf("=== END DEBUG ===\n\n");
}

// =========================================================
// FONCTION DE DEBUG: Afficher le contenu du fichier d'assignation
// =========================================================
void debug_assignment_file() {
    printf("\n=== DEBUG coach_assignment.txt ===\n");
    FILE *f = fopen("coach_assignment.txt", "r");
    if (!f) {
        printf("File does not exist or cannot be opened\n");
        return;
    }
    
    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), f)) {
        line_num++;
        line[strcspn(line, "\n")] = '\0';
        printf("Line %d: %s\n", line_num, line);
    }
    fclose(f);
    printf("=== END DEBUG ===\n\n");
}

// ===========================================
// SEND EMAIL TO ADMIN - FOR ADD/MODIFY/DELETE ACTIONS
// ===========================================
int send_email_to_admin(const char *subject, const char *body, const char *action_type) {
    printf("\n=== DEBUG EMAIL TO ADMIN START (%s) ===\n", action_type);
    printf("Subject: %s\n", subject);
    
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        // Configuration Gmail SMTP
        curl_easy_setopt(curl, CURLOPT_USERNAME, "topgym3b2@gmail.com");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "geutzwptyybqiffa");
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "<topgym3b2@gmail.com>");
        
        // Debug SMTP
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

        // Destinataire - ADMINISTRATION SEULEMENT
        recipients = curl_slist_append(recipients, "<raedboukari2018@gmail.com>");
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Construire le message pour l'administration
        char message[2048];
        const char *action_emoji = "📋";
        const char *action_title = "SYSTEM NOTIFICATION";
        
        if (strcmp(action_type, "add") == 0) {
            action_emoji = "➕";
            action_title = "NEW COACH ADDED";
        } else if (strcmp(action_type, "modify") == 0) {
            action_emoji = "✏️";
            action_title = "COACH MODIFIED";
        } else if (strcmp(action_type, "delete") == 0) {
            action_emoji = "🗑️";
            action_title = "COACH DELETED";
        }
        
        snprintf(message, sizeof(message),
                 "To: Administrator <raedboukari2018@gmail.com>\r\n"
                 "From: TopGym System <topgym3b2@gmail.com>\r\n"
                 "Subject: %s\r\n"
                 "Content-Type: text/plain; charset=UTF-8\r\n"
                 "\r\n"
                 "%s %s %s\r\n"
                 "══════════════════════════════════════════\r\n"
                 "\r\n"
                 "%s\r\n"
                 "\r\n"
                 "📍 This is an automated notification from the TopGym management system.\r\n"
                 "📍 Database has been updated accordingly.\r\n"
                 "\r\n"
                 "📊 System Status: Action completed\r\n"
                 "✅ Action: %s\r\n"
                 "\r\n"
                 "No further action required. This email is for your information only.\r\n"
                 "\r\n"
                 "Best regards,\r\n"
                 "🏋️ TopGym Automated System 🏋️\r\n"
                 "\r\n"
                 ".\r\n",
                 subject, action_emoji, action_title, action_emoji, body, action_type);
        
        printf("DEBUG: Email message size: %ld bytes\n", strlen(message));
        
        const char *email_data = message;
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &email_data);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        
        printf("DEBUG: Starting email transmission to admin...\n");
        
        res = curl_easy_perform(curl);
        
        printf("DEBUG: curl_easy_perform returned: %d\n", res);
        if(res != CURLE_OK) {
            fprintf(stderr, "EMAIL ERROR: %s\n", curl_easy_strerror(res));
        } else {
            printf("✅ SUCCESS: Email sent to admin successfully!\n");
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    } else {
        printf("ERROR: Failed to initialize curl\n");
    }
    
    curl_global_cleanup();
    printf("=== DEBUG EMAIL TO ADMIN END ===\n\n");
    return (res == CURLE_OK);
}

// ===========================================
// SEND EMAIL TO COACH - FOR ASSIGNMENT ONLY (with copy to admin)
// ===========================================
int send_email_to_coach(const char *coach_email, const char *subject, const char *body) {
    printf("\n=== DEBUG EMAIL TO COACH START ===\n");
    printf("Coach Email: %s\n", coach_email);
    printf("Subject: %s\n", subject);
    
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct curl_slist *recipients = NULL;
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    
    if(curl) {
        // Gmail SMTP Configuration
        curl_easy_setopt(curl, CURLOPT_USERNAME, "topgym3b2@gmail.com");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "geutzwptyybqiffa");
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "<topgym3b2@gmail.com>");
        
        // Debug SMTP
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

        // Destinataire - LE COACH ET L'ADMIN EN COPIE
        char coach_email_full[256];
        if (coach_email && strlen(coach_email) > 0) {
            snprintf(coach_email_full, sizeof(coach_email_full), "<%s>", coach_email);
        } else {
            snprintf(coach_email_full, sizeof(coach_email_full), "<coach@topgym.com>");
        }
        
        recipients = curl_slist_append(recipients, coach_email_full);
        recipients = curl_slist_append(recipients, "<raedboukari2018@gmail.com>");
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Construire le message pour le coach (avec admin en copie)
        char message[2048];
        snprintf(message, sizeof(message),
                 "To: Coach %s\r\n"
                 "Cc: Administrator <raedboukari2018@gmail.com>\r\n"
                 "From: TopGym Management <topgym3b2@gmail.com>\r\n"
                 "Subject: %s\r\n"
                 "Content-Type: text/plain; charset=UTF-8\r\n"
                 "\r\n"
                 "🏋️ HELLO DEAR COACH! 🏋️\r\n"
                 "══════════════════════════════════════════\r\n"
                 "\r\n"
                 "📋 ASSIGNMENT CONFIRMATION\r\n"
                 "══════════════════════════════════════════\r\n"
                 "\r\n"
                 "%s\r\n"
                 "\r\n"
                 "📍 IMPORTANT INFORMATION:\r\n"
                 "• Please arrive 15 minutes before the class\r\n"
                 "• Bring your personal equipment if needed\r\n"
                 "• Contact the center if unavailable\r\n"
                 "\r\n"
                 "📞 CENTER CONTACT:\r\n"
                 "• Phone: +216 72 152 458\r\n"
                 "• Email: contact@topgym.com\r\n"
                 "\r\n"
                 "Thank you for your commitment!\r\n"
                 "\r\n"
                 "Best regards,\r\n"
                 "TopGym Management Team\r\n"
                 "🏆 Your success is our priority!\r\n"
                 "\r\n"
                 ".\r\n",
                 coach_email, subject, body);
        
        printf("DEBUG: Email message size: %ld bytes\n", strlen(message));
        
        const char *email_data = message;
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &email_data);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        
        printf("DEBUG: Starting email transmission to coach (with admin copy)...\n");
        
        res = curl_easy_perform(curl);
        
        printf("DEBUG: curl_easy_perform returned: %d\n", res);
        if(res != CURLE_OK) {
            fprintf(stderr, "EMAIL ERROR: %s\n", curl_easy_strerror(res));
        } else {
            printf("✅ SUCCESS: Email sent to coach (with admin copy) successfully!\n");
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    } else {
        printf("ERROR: Failed to initialize curl\n");
    }
    
    curl_global_cleanup();
    printf("=== DEBUG EMAIL TO COACH END ===\n\n");
    return (res == CURLE_OK);
}

// Alias pour compatibilité
int send_email(const char *subject, const char *body) {
    return send_email_to_admin(subject, body, "add");
}

int gender = 1; // 1 = Male, 2 = Female

// =========================================================
// AFFICHER UN MESSAGE DANS UNE POP-UP
// =========================================================
void show_popup_message(GtkWidget *parent, const char *title, const char *message, GtkMessageType type) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL,
        type,
        GTK_BUTTONS_OK,
        "%s", message);
    
    if (title) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// =========================================================
// AFFICHER UNE POP-UP DE CONFIRMATION (YES/NO)
// =========================================================
gboolean show_confirmation_dialog(GtkWidget *parent, const char *title, const char *message) {
    GtkWidget *dialog = gtk_message_dialog_new(
        parent ? GTK_WINDOW(parent) : NULL,
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s", message);
    
    if (title) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    return (response == GTK_RESPONSE_YES);
}

// ===========================================
// CLEAR LABEL
// ===========================================
gboolean clear_label(gpointer data)
{
    gtk_label_set_text(GTK_LABEL(data), "");
    return FALSE;
}

// ===========================================
// VALIDATION INPUT - AVEC CONTRÔLE MAX 8 CHIFFRES
// ===========================================
int is_number(const char *str)
{
    if (str == NULL || str[0] == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++)
        if (!isdigit(str[i])) return 0;
    return 1;
}

int is_valid_id(const char *str)
{
    if (!is_number(str)) return 0;
    if (strlen(str) > 8) return 0;  // Maximum 8 chiffres
    if (strlen(str) == 0) return 0; // Ne peut pas être vide
    return 1;
}

int is_valid_phone(const char *str)
{
    if (!is_number(str)) return 0;
    if (strlen(str) != 8) return 0;  // Doit être exactement 8 chiffres (format Tunisien)
    return 1;
}

int is_letters(const char *str)
{
    if (str == NULL || str[0] == '\0') return 0;
    for (int i = 0; str[i] != '\0'; i++)
        if (!isalpha(str[i]) && str[i] != ' ') return 0;
    return 1;
}

// =========================================================
// CREATE COLUMNS (only once) - MODIFIÉ POUR 10 COLONNES
// =========================================================
void create_columns(GtkTreeView *treeview)
{
    if (g_list_length(gtk_tree_view_get_columns(treeview)) > 0)
        return;

    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    const char *titles[10] = {"ID", "Last Name", "First Name", "Day", "Month", "Year", "Center", "Phone", "Gender", "Specialty"};

    for (int i = 0; i < 10; i++)
    {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_append_column(treeview, column);
    }
}

// =========================================================
// LOAD coaches.txt → TreeView - MODIFIÉ POUR LA SPÉCIALITÉ
// =========================================================
void loadCoaches(GtkTreeView *treeview, const char *filename)
{
    printf("\n=== DEBUG loadCoaches START ===\n");
    printf("Loading from: %s\n", filename);
    
    // Vérifier si des colonnes existent déjà
    if (g_list_length(gtk_tree_view_get_columns(treeview)) > 0)
    {
        GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
        if (store)
        {
            gtk_list_store_clear(store);
        }
        else
        {
            printf("WARNING: Store is NULL\n");
            return;
        }
    }
    else
    {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        const char *titles[10] = {"ID", "Last Name", "First Name", "Day", "Month", "Year", "Center", "Phone", "Gender", "Specialty"};
        
        for (int i = 0; i < 10; i++)
        {
            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
            gtk_tree_view_column_set_resizable(column, TRUE);
            gtk_tree_view_append_column(treeview, column);
        }
    }

    GtkListStore *store;
    GtkTreeIter iter;

    store = gtk_list_store_new(10,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_INT,
                               G_TYPE_INT,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING);

    FILE *f = fopen(filename, "r");
    if (f)
    {
        Coach c;
        char gender_str[10];
        char specialty[30];
        int line_count = 0;
        
        printf("DEBUG: File opened successfully\n");
        
        while (fscanf(f, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                      &c.id, c.lastName, c.firstName,
                      &c.dateOfBirth.day, &c.dateOfBirth.month, &c.dateOfBirth.year,
                      c.center, c.phoneNumber, gender_str, specialty) == 10)
        {
            line_count++;
            printf("DEBUG: Loaded coach %d: %s %s (Specialty: %s)\n", 
                   c.id, c.firstName, c.lastName, specialty);
            
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, c.id,
                               1, c.lastName,
                               2, c.firstName,
                               3, c.dateOfBirth.day,
                               4, c.dateOfBirth.month,
                               5, c.dateOfBirth.year,
                               6, c.center,
                               7, c.phoneNumber,
                               8, gender_str,
                               9, specialty,
                               -1);
        }
        fclose(f);
        
        printf("DEBUG: Total coaches loaded: %d\n", line_count);
        
        if (line_count == 0) {
            printf("WARNING: File exists but no coaches loaded\n");
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, -1,
                               1, "No coaches found",
                               2, "",
                               3, 0,
                               4, 0,
                               5, 0,
                               6, "",
                               7, "",
                               8, "",
                               9, "",
                               -1);
        }
    }
    else
    {
        printf("ERROR: Cannot open file %s (errno: %d)\n", filename, errno);
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, -1,
                           1, "coach.txt not found",
                           2, "",
                           3, 0,
                           4, 0,
                           5, 0,
                           6, "",
                           7, "",
                           8, "",
                           9, "",
                           -1);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    printf("=== DEBUG loadCoaches END ===\n");
}

// =========================================================
// REFRESH TREEVIEW
// =========================================================
void refresh_tree(GtkWidget *treeview)
{
    printf("\n=== DEBUG refresh_tree ===\n");
    loadCoaches(GTK_TREE_VIEW(treeview), "coach.txt");
}

// =========================================================
// LOAD COURSES → TreeView (7 COLONNES)
// =========================================================
void loadCourses(GtkTreeView *treeview, const char *filename)
{
    printf("\n=== DEBUG loadCourses START ===\n");
    
    if (g_list_length(gtk_tree_view_get_columns(treeview)) > 0)
    {
        GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(treeview));
        if (store)
        {
            gtk_list_store_clear(store);
        }
        else
        {
            return;
        }
    }
    else
    {
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        
        const char *titles[7] = {"ID", "Name", "Type", "Center", "Time", "Equipment", "Capacity"};
        
        for (int i = 0; i < 7; i++)
        {
            renderer = gtk_cell_renderer_text_new();
            column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
            gtk_tree_view_column_set_resizable(column, TRUE);
            gtk_tree_view_append_column(treeview, column);
        }
    }

    GtkListStore *store;
    GtkTreeIter iter;

    store = gtk_list_store_new(7,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING);

    FILE *f = fopen(filename, "r");
    if (f)
    {
        char line[256];
        if (fgets(line, sizeof(line), f) != NULL)
        {
            printf("DEBUG: Skipping header: %s", line);
            
            while (fgets(line, sizeof(line), f))
            {
                line[strcspn(line, "\n")] = '\0';
                
                char course_id[50], course_name[50], course_type[50];
                char course_center[50], course_time[50], equipment[50], capacity[50];
                
                int fields = sscanf(line, "%49s %49s %49s %49s %49s %49s %49s", 
                                   course_id, course_name, course_type, 
                                   course_center, course_time, equipment, capacity);
                
                if (fields == 7)
                {
                    gtk_list_store_append(store, &iter);
                    gtk_list_store_set(store, &iter,
                                       0, course_id,
                                       1, course_name,
                                       2, course_type,
                                       3, course_center,
                                       4, course_time,
                                       5, equipment,
                                       6, capacity,
                                       -1);
                }
            }
        }
        fclose(f);
    }
    else
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, "ERROR",
                           1, "courses.txt not found",
                           2, "",
                           3, "",
                           4, "",
                           5, "",
                           6, "",
                           -1);
    }

    gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    printf("=== DEBUG loadCourses END ===\n");
}

// =========================================================
// REFRESH COURSE TREEVIEW
// =========================================================
void refresh_course_tree(GtkWidget *treeview)
{
    loadCourses(GTK_TREE_VIEW(treeview), "courses.txt");
}

// =========================================================
// Mettre à jour la capacité d'un cours
// =========================================================
int update_course_capacity(const char *course_id, int change_amount)
{
    if (!course_id || strlen(course_id) == 0) {
        return 0;
    }
    
    FILE *f = fopen("courses.txt", "r");
    if (!f) {
        printf("ERROR: Cannot open courses.txt for reading\n");
        return 0;
    }
    
    char lines[100][256];
    int line_count = 0;
    char line[256];
    
    if (fgets(line, sizeof(line), f)) {
        strncpy(lines[line_count++], line, sizeof(lines[0])-1);
    }
    
    int updated = 0;
    
    while (fgets(line, sizeof(line), f)) {
        char current_course_id[50], course_name[50], course_type[50];
        char course_center[50], course_time[50], equipment[50], capacity_text[50];
        
        line[strcspn(line, "\n")] = '\0';
        
        int fields = sscanf(line, "%49s %49s %49s %49s %49s %49s %49s", 
                           current_course_id, course_name, course_type, 
                           course_center, course_time, equipment, capacity_text);
        
        if (fields == 7) {
            gchar *clean_current_id = g_strdup(current_course_id);
            gchar *clean_search_id = g_strdup(course_id);
            
            if (clean_current_id && clean_search_id) {
                g_strstrip(clean_current_id);
                g_strstrip(clean_search_id);
                
                if (strcmp(clean_current_id, clean_search_id) == 0) {
                    int current_capacity = atoi(capacity_text);
                    int new_capacity = current_capacity + change_amount;
                    
                    if (new_capacity < 0) {
                        new_capacity = 0;
                    }
                    
                    snprintf(lines[line_count], sizeof(lines[line_count]),
                            "%s %s %s %s %s %s %d",
                            current_course_id, course_name, course_type,
                            course_center, course_time, equipment, new_capacity);
                    
                    printf("DEBUG: Updated capacity for course %s: %d -> %d\n",
                           current_course_id, current_capacity, new_capacity);
                    
                    updated = 1;
                } else {
                    strncpy(lines[line_count], line, sizeof(lines[line_count])-1);
                }
                
                g_free(clean_current_id);
                g_free(clean_search_id);
            } else {
                strncpy(lines[line_count], line, sizeof(lines[line_count])-1);
            }
        } else {
            strncpy(lines[line_count], line, sizeof(lines[line_count])-1);
        }
        
        line_count++;
        if (line_count >= 100) {
            break;
        }
    }
    
    fclose(f);
    
    if (updated) {
        FILE *f_out = fopen("courses.txt", "w");
        if (!f_out) {
            printf("ERROR: Cannot open courses.txt for writing\n");
            return 0;
        }
        
        for (int i = 0; i < line_count; i++) {
            fprintf(f_out, "%s\n", lines[i]);
        }
        
        fclose(f_out);
        printf("DEBUG: Successfully updated courses.txt\n");
        return 1;
    }
    
    return 0;
}

// =========================================================
// Compter les coachs assignés à un cours - CORRIGÉ
// =========================================================
int count_assigned_coaches(const char *course_id)
{
    if (!course_id || strlen(course_id) == 0) {
        return 0;
    }
    
    FILE *f = fopen("coach_assignment.txt", "r");
    if (!f) {
        printf("DEBUG: coach_assignment.txt not found or empty\n");
        return 0;
    }
    
    int count = 0;
    char line[256];
    
    gchar *clean_search_id = g_strdup(course_id);
    if (clean_search_id) {
        g_strstrip(clean_search_id);
        printf("DEBUG: Searching for course ID: '%s'\n", clean_search_id);
        
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = '\0';
            
            // Parser la ligne selon le nouveau format
            // Format: coach_id|coach_name|coach_phone|specialty|day|month|year|course_id|course_name|course_center|course_time
            int parsed_coach_id, parsed_day, parsed_month, parsed_year;
            char parsed_coach_name[100], parsed_coach_phone[20], parsed_specialty[30];
            char parsed_course_id[10], parsed_course_name[50], parsed_course_center[30], parsed_course_time[20];
            
            int parsed_fields = sscanf(line, "%d|%99[^|]|%19[^|]|%29[^|]|%d|%d|%d|%9[^|]|%49[^|]|%29[^|]|%19[^|]",
                                      &parsed_coach_id, parsed_coach_name, parsed_coach_phone, parsed_specialty,
                                      &parsed_day, &parsed_month, &parsed_year,
                                      parsed_course_id, parsed_course_name, parsed_course_center, parsed_course_time);
            
            if (parsed_fields == 11) {
                if (strcmp(parsed_course_id, clean_search_id) == 0) {
                    count++;
                    printf("DEBUG: Found assignment #%d for course %s\n", count, parsed_course_id);
                }
            } else {
                // Essayer un parsing simple
                char *token;
                char *rest = line;
                int field = 0;
                
                while ((token = strtok_r(rest, "|", &rest))) {
                    field++;
                    if (field == 8) { // 8ème champ = course_id
                        if (strcmp(token, clean_search_id) == 0) {
                            count++;
                        }
                        break;
                    }
                }
            }
        }
        
        g_free(clean_search_id);
    }
    
    fclose(f);
    printf("DEBUG: Total assignments for course %s: %d\n", course_id, count);
    return count;
}

// =========================================================
// ADD COACH - AVEC CONTRÔLE MAX 8 CHIFFRES
// =========================================================
void on_btadd_clicked(GtkButton *button, gpointer user_data)
{
    printf("\n=== DEBUG on_btadd_clicked START ===\n");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));
    if (!win) {
        printf("ERROR: Could not get toplevel window\n");
        show_popup_message(win, "Error", "Could not get toplevel window", GTK_MESSAGE_ERROR);
        return;
    }
    
    GtkWidget *entryid = lookup_widget(win, "entryid");
    GtkWidget *entryname = lookup_widget(win, "entryname2");
    GtkWidget *entryfirst = lookup_widget(win, "entryfirstname");
    GtkWidget *entryphone = lookup_widget(win, "entryphonenumber");
    GtkWidget *spin_day = lookup_widget(win, "spinbuttonday");
    GtkWidget *spin_month = lookup_widget(win, "spinbuttonmonth");
    GtkWidget *spin_year = lookup_widget(win, "spinbuttonyear");
    GtkWidget *combo_center = lookup_widget(win, "entrycentre2");
    GtkWidget *combo_specialty = lookup_widget(win, "comboboxentry_spec");

    if (!entryid || !entryname || !entryfirst || !entryphone || !spin_day || 
        !spin_month || !spin_year || !combo_center || !combo_specialty)
    {
        printf("ERROR: Some widgets are NULL\n");
        show_popup_message(win, "Error", "Some widgets are missing", GTK_MESSAGE_ERROR);
        return;
    }
    
    const gchar *id = gtk_entry_get_text(GTK_ENTRY(entryid));
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(entryname));
    const gchar *fname = gtk_entry_get_text(GTK_ENTRY(entryfirst));
    const gchar *phone = gtk_entry_get_text(GTK_ENTRY(entryphone));
    
    printf("DEBUG: Input values - ID: '%s', Name: '%s', First: '%s', Phone: '%s'\n", 
           id, name, fname, phone);
    
    // Validation avec contrôle max 8 chiffres
    if (!id || !is_valid_id(id)) { 
        show_popup_message(win, "Validation Error", "ID must be numbers only (max 8 digits)", GTK_MESSAGE_WARNING);
        return; 
    }
    if (!name || !is_letters(name)) { 
        show_popup_message(win, "Validation Error", "Name must be letters only", GTK_MESSAGE_WARNING);
        return; 
    }
    if (!fname || !is_letters(fname)) { 
        show_popup_message(win, "Validation Error", "First name must be letters only", GTK_MESSAGE_WARNING);
        return; 
    }
    if (!phone || !is_valid_phone(phone)) { 
        show_popup_message(win, "Validation Error", "Phone must be exactly 8 numbers (ex: 12345678)", GTK_MESSAGE_WARNING);
        return; 
    }
    
    // Check if ID already exists
    FILE *f = fopen("coach.txt", "r");
    if (f) {
        int existing_id;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), f)) {
            sscanf(buffer, "%d", &existing_id);
            if (existing_id == atoi(id)) {
                fclose(f);
                show_popup_message(win, "Error", "ID already exists!", GTK_MESSAGE_ERROR);
                return;
            }
        }
        fclose(f);
    }
    
    Coach c;
    c.id = atoi(id);
    
    strncpy(c.lastName, name, sizeof(c.lastName)-1);
    c.lastName[sizeof(c.lastName)-1] = '\0';
    strncpy(c.firstName, fname, sizeof(c.firstName)-1);
    c.firstName[sizeof(c.firstName)-1] = '\0';
    strncpy(c.phoneNumber, phone, sizeof(c.phoneNumber)-1);
    c.phoneNumber[sizeof(c.phoneNumber)-1] = '\0';
    
    c.dateOfBirth.day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_day));
    c.dateOfBirth.month = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_month));
    c.dateOfBirth.year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_year));
    
    // Centre
    GtkEntry *entry_center = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_center)));
    const gchar *center_text = gtk_entry_get_text(entry_center);
    if (center_text == NULL || strlen(center_text) == 0) {
        show_popup_message(win, "Error", "Please select a center!", GTK_MESSAGE_WARNING);
        return;
    }
    strncpy(c.center, center_text, sizeof(c.center)-1);
    c.center[sizeof(c.center)-1] = '\0';
    
    // Spécialité
    GtkEntry *entry_specialty = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_specialty)));
    const gchar *specialty_text = gtk_entry_get_text(entry_specialty);
    if (specialty_text == NULL || strlen(specialty_text) == 0) {
        show_popup_message(win, "Error", "Please select a specialty!", GTK_MESSAGE_WARNING);
        return;
    }
    strncpy(c.specialty, specialty_text, sizeof(c.specialty)-1);
    c.specialty[sizeof(c.specialty)-1] = '\0';
    
    c.gender = gender;
    
    printf("DEBUG: Attempting to add coach - ID: %d, Name: %s %s, Specialty: %s\n", 
           c.id, c.firstName, c.lastName, c.specialty);
    
    if (addCoach("coach.txt", c)) {
        show_popup_message(win, "Success", "Coach added successfully!", GTK_MESSAGE_INFO);
        printf("SUCCESS: Coach added to database\n");
        
        // Debug le fichier
        debug_coach_file();
        
        // Email
        char email_body[512];
        snprintf(email_body, sizeof(email_body),
                 "New Coach Information:\n"
                 "• ID: %d\n"
                 "• Name: %s %s\n"
                 "• Center: %s\n"
                 "• Phone: %s\n"
                 "• Gender: %s\n"
                 "• Specialty: %s\n"
                 "• Date of Birth: %d/%d/%d",
                 c.id, c.firstName, c.lastName,
                 c.center, c.phoneNumber,
                 (c.gender == 1) ? "Male" : "Female",
                 c.specialty,
                 c.dateOfBirth.day, c.dateOfBirth.month, c.dateOfBirth.year);
        
        send_email_to_admin("New Coach Added", email_body, "add");
        
    } else {
        show_popup_message(win, "Error", "Error adding coach!", GTK_MESSAGE_ERROR);
        printf("ERROR: Failed to add coach\n");
    }

    // Clear fields
    gtk_entry_set_text(GTK_ENTRY(entryid), "");
    gtk_entry_set_text(GTK_ENTRY(entryname), "");
    gtk_entry_set_text(GTK_ENTRY(entryfirst), "");
    gtk_entry_set_text(GTK_ENTRY(entryphone), "");
    gtk_entry_set_text(entry_center, "");
    gtk_entry_set_text(entry_specialty, "");

    // Refresh tree view
    GtkWidget *tree = lookup_widget(win, "treeviewmanage");
    if (tree) {
        refresh_tree(tree);
    }
    
    printf("=== DEBUG on_btadd_clicked END ===\n\n");
}

// =========================================================
// MODIFY COACH - AVEC CONFIRMATION YES/NO ET CONTRÔLE MAX 8 CHIFFRES
// =========================================================
void on_btmodify_clicked(GtkButton *button, gpointer user_data)
{
    printf("\n=== DEBUG on_btmodify_clicked START ===\n");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));

    GtkWidget *entryid = lookup_widget(win, "entryid");
    GtkWidget *entryname = lookup_widget(win, "entryname2");
    GtkWidget *entryfirst = lookup_widget(win, "entryfirstname");
    GtkWidget *entryphone = lookup_widget(win, "entryphonenumber");
    GtkWidget *spin_day = lookup_widget(win, "spinbuttonday");
    GtkWidget *spin_month = lookup_widget(win, "spinbuttonmonth");
    GtkWidget *spin_year = lookup_widget(win, "spinbuttonyear");
    GtkWidget *combo_center = lookup_widget(win, "entrycentre2");
    GtkWidget *combo_specialty = lookup_widget(win, "comboboxentry_spec");

    if (!entryid || !entryname || !entryfirst || !entryphone || !spin_day || 
        !spin_month || !spin_year || !combo_center || !combo_specialty)
    {
        show_popup_message(win, "Error", "Some widgets are missing", GTK_MESSAGE_ERROR);
        return;
    }

    const gchar *id = gtk_entry_get_text(GTK_ENTRY(entryid));
    if (!is_valid_id(id)) { 
        show_popup_message(win, "Validation Error", "ID must be numbers only (max 8 digits)", GTK_MESSAGE_WARNING);
        return; 
    }

    // Check if coach exists before showing confirmation
    int coach_id = atoi(id);
    int coach_exists = 0;
    FILE *check_file = fopen("coach.txt", "r");
    if (check_file) {
        int temp_id;
        char line[256];
        while (fgets(line, sizeof(line), check_file)) {
            if (sscanf(line, "%d", &temp_id) == 1 && temp_id == coach_id) {
                coach_exists = 1;
                break;
            }
        }
        fclose(check_file);
    }
    
    if (!coach_exists) {
        show_popup_message(win, "Error", "Coach ID not found!", GTK_MESSAGE_ERROR);
        return;
    }

    // Get old coach info before modification for email
    Coach old_coach;
    int found_old = 0;
    FILE *f = fopen("coach.txt", "r");
    if (f) {
        Coach temp;
        char gender_str[10];
        char specialty_str[30];
        while (fscanf(f, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                      &temp.id, temp.lastName, temp.firstName,
                      &temp.dateOfBirth.day, &temp.dateOfBirth.month, &temp.dateOfBirth.year,
                      temp.center, temp.phoneNumber, gender_str, specialty_str) == 10) {
            if (temp.id == coach_id) {
                old_coach = temp;
                old_coach.gender = (strcmp(gender_str, "Male") == 0) ? 1 : 2;
                strncpy(old_coach.specialty, specialty_str, sizeof(old_coach.specialty)-1);
                old_coach.specialty[sizeof(old_coach.specialty)-1] = '\0';
                found_old = 1;
                break;
            }
        }
        fclose(f);
    }

    Coach c;
    c.id = atoi(id);

    strncpy(c.lastName, gtk_entry_get_text(GTK_ENTRY(entryname)), sizeof(c.lastName)-1);
    c.lastName[sizeof(c.lastName)-1] = '\0';
    strncpy(c.firstName, gtk_entry_get_text(GTK_ENTRY(entryfirst)), sizeof(c.firstName)-1);
    c.firstName[sizeof(c.firstName)-1] = '\0';
    
    const gchar *phone = gtk_entry_get_text(GTK_ENTRY(entryphone));
    if (!is_valid_phone(phone)) { 
        show_popup_message(win, "Validation Error", "Phone must be exactly 8 numbers (ex: 12345678)", GTK_MESSAGE_WARNING);
        return; 
    }
    strncpy(c.phoneNumber, phone, sizeof(c.phoneNumber)-1);
    c.phoneNumber[sizeof(c.phoneNumber)-1] = '\0';

    c.dateOfBirth.day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_day));
    c.dateOfBirth.month = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_month));
    c.dateOfBirth.year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_year));

    // Centre
    GtkEntry *entry_center = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_center)));
    const gchar *center_text = gtk_entry_get_text(entry_center);
    if (center_text == NULL || strlen(center_text) == 0) {
        show_popup_message(win, "Error", "Please select a center!", GTK_MESSAGE_WARNING);
        return;
    }
    strncpy(c.center, center_text, sizeof(c.center)-1);
    c.center[sizeof(c.center)-1] = '\0';
    
    // Spécialité
    GtkEntry *entry_specialty = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_specialty)));
    const gchar *specialty_text = gtk_entry_get_text(entry_specialty);
    if (specialty_text == NULL || strlen(specialty_text) == 0) {
        show_popup_message(win, "Error", "Please select a specialty!", GTK_MESSAGE_WARNING);
        return;
    }
    strncpy(c.specialty, specialty_text, sizeof(c.specialty)-1);
    c.specialty[sizeof(c.specialty)-1] = '\0';

    c.gender = gender;

    // Show confirmation dialog
    gchar *confirmation_message = g_strdup_printf(
        "Are you sure you want to modify coach ID %d?\n\n"
        "New Information:\n"
        "• Name: %s %s\n"
        "• Center: %s\n"
        "• Phone: %s\n"
        "• Specialty: %s",
        c.id, c.firstName, c.lastName,
        c.center, c.phoneNumber, c.specialty);
    
    gboolean confirmed = show_confirmation_dialog(win, "Confirm Modification", confirmation_message);
    g_free(confirmation_message);
    
    if (!confirmed) {
        printf("DEBUG: Modification cancelled by user\n");
        return;
    }

    printf("DEBUG: Attempting to modify coach ID %d, Specialty: %s\n", c.id, c.specialty);
    
    if (modifyCoach("coach.txt", c.id, c)) {
        show_popup_message(win, "Success", "Coach modified successfully!", GTK_MESSAGE_INFO);
        printf("SUCCESS: Coach modified\n");
        
        debug_coach_file();
        
        // Email
        char email_body[768];
        if (found_old) {
            snprintf(email_body, sizeof(email_body),
                     "Coach Modified - Changes:\n\n"
                     "BEFORE MODIFICATION:\n"
                     "• ID: %d\n"
                     "• Name: %s %s\n"
                     "• Center: %s\n"
                     "• Phone: %s\n"
                     "• Gender: %s\n"
                     "• Specialty: %s\n"
                     "• Date of Birth: %d/%d/%d\n\n"
                     "AFTER MODIFICATION:\n"
                     "• ID: %d\n"
                     "• Name: %s %s\n"
                     "• Center: %s\n"
                     "• Phone: %s\n"
                     "• Gender: %s\n"
                     "• Specialty: %s\n"
                     "• Date of Birth: %d/%d/%d",
                     old_coach.id, old_coach.firstName, old_coach.lastName,
                     old_coach.center, old_coach.phoneNumber,
                     (old_coach.gender == 1) ? "Male" : "Female",
                     old_coach.specialty,
                     old_coach.dateOfBirth.day, old_coach.dateOfBirth.month, old_coach.dateOfBirth.year,
                     c.id, c.firstName, c.lastName,
                     c.center, c.phoneNumber,
                     (c.gender == 1) ? "Male" : "Female",
                     c.specialty,
                     c.dateOfBirth.day, c.dateOfBirth.month, c.dateOfBirth.year);
        } else {
            snprintf(email_body, sizeof(email_body),
                     "Coach Modified (old info not found):\n"
                     "• ID: %d\n"
                     "• Name: %s %s\n"
                     "• Center: %s\n"
                     "• Phone: %s\n"
                     "• Gender: %s\n"
                     "• Specialty: %s\n"
                     "• Date of Birth: %d/%d/%d",
                     c.id, c.firstName, c.lastName,
                     c.center, c.phoneNumber,
                     (c.gender == 1) ? "Male" : "Female",
                     c.specialty,
                     c.dateOfBirth.day, c.dateOfBirth.month, c.dateOfBirth.year);
        }
        
        send_email_to_admin("Coach Modified", email_body, "modify");
        
    } else {
        show_popup_message(win, "Error", "Error! ID not found!", GTK_MESSAGE_ERROR);
        printf("ERROR: Coach not found\n");
    }

    GtkWidget *tree = lookup_widget(win, "treeviewmanage");
    if (tree) refresh_tree(tree);
    
    printf("=== DEBUG on_btmodify_clicked END ===\n\n");
}

// =========================================================
// DELETE COACH - AVEC CONFIRMATION YES/NO
// =========================================================
void on_btdelete_clicked(GtkButton *button, gpointer user_data)
{
    printf("\n=== DEBUG on_btdelete_clicked START ===\n");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *entryid = lookup_widget(win, "entryid");

    if (!entryid) {
        show_popup_message(win, "Error", "Entry ID widget not found", GTK_MESSAGE_ERROR);
        return;
    }

    const gchar *id = gtk_entry_get_text(GTK_ENTRY(entryid));
    if (!is_valid_id(id)) { 
        show_popup_message(win, "Validation Error", "ID must be numbers only (max 8 digits)", GTK_MESSAGE_WARNING);
        return; 
    }

    int coach_id = atoi(id);
    
    // Check if coach exists before showing confirmation
    int coach_exists = 0;
    char coach_name[100] = "";
    FILE *check_file = fopen("coach.txt", "r");
    if (check_file) {
        int temp_id;
        char temp_fname[30], temp_lname[30];
        char line[256];
        while (fgets(line, sizeof(line), check_file)) {
            if (sscanf(line, "%d %29s %29s", &temp_id, temp_lname, temp_fname) >= 3) {
                if (temp_id == coach_id) {
                    coach_exists = 1;
                    snprintf(coach_name, sizeof(coach_name), "%s %s", temp_fname, temp_lname);
                    break;
                }
            }
        }
        fclose(check_file);
    }
    
    if (!coach_exists) {
        show_popup_message(win, "Error", "Coach ID not found!", GTK_MESSAGE_ERROR);
        return;
    }
    
    // Get coach info before deletion for email
    Coach deleted_coach;
    int found_coach = 0;
    FILE *f = fopen("coach.txt", "r");
    if (f) {
        Coach temp;
        char gender_str[10];
        char specialty_str[30];
        while (fscanf(f, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                      &temp.id, temp.lastName, temp.firstName,
                      &temp.dateOfBirth.day, &temp.dateOfBirth.month, &temp.dateOfBirth.year,
                      temp.center, temp.phoneNumber, gender_str, specialty_str) == 10) {
            if (temp.id == coach_id) {
                deleted_coach = temp;
                deleted_coach.gender = (strcmp(gender_str, "Male") == 0) ? 1 : 2;
                strncpy(deleted_coach.specialty, specialty_str, sizeof(deleted_coach.specialty)-1);
                deleted_coach.specialty[sizeof(deleted_coach.specialty)-1] = '\0';
                found_coach = 1;
                break;
            }
        }
        fclose(f);
    }

    // Show confirmation dialog
    gchar *confirmation_message = g_strdup_printf(
        "Are you sure you want to DELETE coach ID %d?\n\n"
        "Coach: %s\n"
        "This action cannot be undone!",
        coach_id, coach_name);
    
    gboolean confirmed = show_confirmation_dialog(win, "Confirm Deletion", confirmation_message);
    g_free(confirmation_message);
    
    if (!confirmed) {
        printf("DEBUG: Deletion cancelled by user\n");
        return;
    }

    printf("DEBUG: Attempting to delete coach ID %d\n", coach_id);
    
    if (deleteCoach("coach.txt", coach_id)) {
        show_popup_message(win, "Success", "Coach deleted successfully!", GTK_MESSAGE_INFO);
        printf("SUCCESS: Coach deleted\n");
        
        debug_coach_file();
        
        // Email
        if (found_coach) {
            char email_body[512];
            snprintf(email_body, sizeof(email_body),
                     "Coach Deleted - Removed Information:\n"
                     "• ID: %d\n"
                     "• Name: %s %s\n"
                     "• Center: %s\n"
                     "• Phone: %s\n"
                     "• Gender: %s\n"
                     "• Specialty: %s\n"
                     "• Date of Birth: %d/%d/%d\n\n"
                     "This coach has been permanently removed from the system.",
                     deleted_coach.id, deleted_coach.firstName, deleted_coach.lastName,
                     deleted_coach.center, deleted_coach.phoneNumber,
                     (deleted_coach.gender == 1) ? "Male" : "Female",
                     deleted_coach.specialty,
                     deleted_coach.dateOfBirth.day, deleted_coach.dateOfBirth.month, deleted_coach.dateOfBirth.year);
            
            send_email_to_admin("Coach Deleted", email_body, "delete");
        }
        
    } else {
        show_popup_message(win, "Error", "Error! ID not found!", GTK_MESSAGE_ERROR);
        printf("ERROR: Coach not found\n");
    }

    // Clear fields
    GtkWidget *entryname = lookup_widget(win, "entryname2");
    GtkWidget *entryfirst = lookup_widget(win, "entryfirstname");
    GtkWidget *entryphone = lookup_widget(win, "entryphonenumber");
    GtkWidget *combo_center = lookup_widget(win, "entrycentre2");
    GtkWidget *combo_specialty = lookup_widget(win, "comboboxentry_spec");
    
    if (entryname) gtk_entry_set_text(GTK_ENTRY(entryname), "");
    if (entryfirst) gtk_entry_set_text(GTK_ENTRY(entryfirst), "");
    if (entryphone) gtk_entry_set_text(GTK_ENTRY(entryphone), "");
    if (entryid) gtk_entry_set_text(GTK_ENTRY(entryid), "");
    if (combo_center) {
        GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_center)));
        gtk_entry_set_text(entry, "");
    }
    if (combo_specialty) {
        GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_specialty)));
        gtk_entry_set_text(entry, "");
    }

    GtkWidget *tree = lookup_widget(win, "treeviewmanage");
    if (tree) refresh_tree(tree);
    
    printf("=== DEBUG on_btdelete_clicked END ===\n\n");
}

// =========================================================
// CHECKBUTTON NAME TOGGLED
// =========================================================
void on_checkbutton_name_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(togglebutton);
    printf("DEBUG: Name filter %s\n", active ? "ACTIVATED" : "DEACTIVATED");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
    GtkWidget *entry_name = lookup_widget(win, "entryname1");
    
    if (entry_name) {
        gtk_widget_set_sensitive(entry_name, active);
    }
}

// =========================================================
// SEARCH COACH - MODIFIÉ pour afficher les résultats dans la TreeView
// =========================================================
void on_btsearch_clicked(GtkButton *button, gpointer user_data)
{
    printf("\n=== DEBUG on_btsearch_clicked START ===\n");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));

    GtkWidget *entryname = lookup_widget(win, "entryname1");
    GtkWidget *combo = lookup_widget(win, "entrycentre1");
    GtkWidget *checkbutton_name = lookup_widget(win, "checkbutton_name");
    GtkWidget *checkbutton_center = lookup_widget(win, "checkbutton_center");
    GtkWidget *treeviewmanage = lookup_widget(win, "treeviewmanage");

    if (!entryname || !combo || !checkbutton_name || !checkbutton_center || !treeviewmanage) {
        printf("ERROR: Some widgets not found!\n");
        show_popup_message(win, "Error", "Some widgets are missing", GTK_MESSAGE_ERROR);
        return;
    }

    gboolean search_by_name = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_name));
    gboolean search_by_center = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkbutton_center));
    
    printf("DEBUG: Search filters - Name: %d, Center: %d\n", search_by_name, search_by_center);

    const gchar *name = NULL;
    const gchar *center = NULL;
    
    gchar *clean_name = NULL;
    gchar *clean_center = NULL;
    
    if (search_by_name) {
        name = gtk_entry_get_text(GTK_ENTRY(entryname));
        if (name && strlen(name) > 0) {
            clean_name = g_strdup(name);
            g_strstrip(clean_name);
            printf("DEBUG: Searching by name: '%s'\n", clean_name);
        } else {
            printf("DEBUG: Name filter active but name is empty\n");
            search_by_name = FALSE;
        }
    }
    
    if (search_by_center) {
        GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo)));
        center = gtk_entry_get_text(entry);
        if (center && strlen(center) > 0) {
            clean_center = g_strdup(center);
            g_strstrip(clean_center);
            printf("DEBUG: Searching by center: '%s'\n", clean_center);
        } else {
            printf("DEBUG: Center filter active but center is empty\n");
            search_by_center = FALSE;
        }
    }
    
    if (!search_by_name && !search_by_center) {
        // Si aucun filtre n'est activé, recharger tous les coachs
        refresh_tree(treeviewmanage);
        show_popup_message(win, "Info", "Showing all coaches", GTK_MESSAGE_INFO);
        
        if (clean_name) g_free(clean_name);
        if (clean_center) g_free(clean_center);
        return;
    }
    
    // Récupérer les valeurs de recherche
    const char *search_name = (search_by_name && clean_name && strlen(clean_name) > 0) ? clean_name : NULL;
    const char *search_center = (search_by_center && clean_center && strlen(clean_center) > 0) ? clean_center : NULL;
    
    // Ouvrir le fichier coach.txt et filtrer les résultats
    FILE *f = fopen("coach.txt", "r");
    if (!f) {
        printf("ERROR: Cannot open coach.txt\n");
        show_popup_message(win, "Error", "Cannot open coach database file", GTK_MESSAGE_ERROR);
        
        if (clean_name) g_free(clean_name);
        if (clean_center) g_free(clean_center);
        return;
    }
    
    // Vider la TreeView actuelle
    GtkListStore *store = gtk_list_store_new(10,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_INT,
                               G_TYPE_INT,
                               G_TYPE_INT,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING,
                               G_TYPE_STRING);
    
    GtkTreeIter iter;
    int found_count = 0;
    char line[256];
    
    // Lire tous les coachs et appliquer les filtres
    while (fgets(line, sizeof(line), f)) {
        Coach c;
        char gender_str[10];
        char specialty[30];
        
        int fields = sscanf(line, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                          &c.id, c.lastName, c.firstName,
                          &c.dateOfBirth.day, &c.dateOfBirth.month, &c.dateOfBirth.year,
                          c.center, c.phoneNumber, gender_str, specialty);
        
        if (fields == 10) {
            c.gender = (strcmp(gender_str, "Male") == 0) ? 1 : 2;
            strncpy(c.specialty, specialty, sizeof(c.specialty)-1);
            c.specialty[sizeof(c.specialty)-1] = '\0';
            
            // Appliquer les filtres
            gboolean match = TRUE;
            
            // Filtre par nom
            if (search_by_name && search_name) {
                char full_name[100];
                snprintf(full_name, sizeof(full_name), "%s %s", c.firstName, c.lastName);
                gchar *lower_full_name = g_ascii_strdown(full_name, -1);
                gchar *lower_search = g_ascii_strdown(search_name, -1);
                
                if (strstr(lower_full_name, lower_search) == NULL) {
                    match = FALSE;
                }
                
                g_free(lower_full_name);
                g_free(lower_search);
            }
            
            // Filtre par centre
            if (match && search_by_center && search_center) {
                gchar *lower_center = g_ascii_strdown(c.center, -1);
                gchar *lower_search_center = g_ascii_strdown(search_center, -1);
                
                if (strcmp(lower_center, lower_search_center) != 0) {
                    match = FALSE;
                }
                
                g_free(lower_center);
                g_free(lower_search_center);
            }
            
            // Si le coach correspond aux critères, l'ajouter à la TreeView
            if (match) {
                found_count++;
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter,
                                   0, c.id,
                                   1, c.lastName,
                                   2, c.firstName,
                                   3, c.dateOfBirth.day,
                                   4, c.dateOfBirth.month,
                                   5, c.dateOfBirth.year,
                                   6, c.center,
                                   7, c.phoneNumber,
                                   8, gender_str,
                                   9, c.specialty,
                                   -1);
            }
        }
    }
    
    fclose(f);
    
    // Si aucun résultat n'est trouvé
    if (found_count == 0) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, -1,
                           1, "No coaches found",
                           2, "",
                           3, 0,
                           4, 0,
                           5, 0,
                           6, "",
                           7, "",
                           8, "",
                           9, "",
                           -1);
        
        gchar *message = g_strdup_printf("No coaches found with the specified criteria.");
        show_popup_message(win, "No Results", message, GTK_MESSAGE_INFO);
        g_free(message);
    } else {
        gchar *message = g_strdup_printf("Found %d coach(es) matching your criteria.", found_count);
        show_popup_message(win, "Search Results", message, GTK_MESSAGE_INFO);
        g_free(message);
    }
    
    // Mettre à jour la TreeView avec les résultats filtrés
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeviewmanage), GTK_TREE_MODEL(store));
    g_object_unref(store);
    
    // Nettoyer la mémoire
    if (clean_name) g_free(clean_name);
    if (clean_center) g_free(clean_center);
    
    printf("DEBUG: Search complete. Found %d coach(es)\n", found_count);
    printf("=== DEBUG on_btsearch_clicked END ===\n\n");
}

// =========================================================
// RADIO BUTTONS
// =========================================================
void on_radiobuttonman_toggled(GtkToggleButton *t, gpointer data)
{
    if (gtk_toggle_button_get_active(t)) gender = 1;
}

void on_radiobuttonwomen_toggled(GtkToggleButton *t, gpointer data)
{
    if (gtk_toggle_button_get_active(t)) gender = 2;
}

// =========================================================
// ROW ACTIVATED - MODIFIÉ POUR LA SPÉCIALITÉ
// =========================================================
void on_treeviewmanage_row_activated(GtkTreeView *treeview,
                                     GtkTreePath *path,
                                     GtkTreeViewColumn *column,
                                     gpointer userdata)
{
    printf("\n=== DEBUG on_treeviewmanage_row_activated START ===\n");
    
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);

    int id, day, month, year;
    gchar *lname = NULL;
    gchar *fname = NULL;
    gchar *center = NULL;
    gchar *phone = NULL;
    gchar *gender_str = NULL;
    gchar *specialty = NULL;

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_tree_model_get(model, &iter,
                           0, &id,
                           1, &lname,
                           2, &fname,
                           3, &day,
                           4, &month,
                           5, &year,
                           6, &center,
                           7, &phone,
                           8, &gender_str,
                           9, &specialty,
                           -1);

        printf("DEBUG: Row activated - ID: %d, Name: %s %s, Specialty: %s\n", 
               id, fname ? fname : "NULL", lname ? lname : "NULL", specialty ? specialty : "NULL");

        GtkWidget *window = lookup_widget(GTK_WIDGET(treeview), "raed_manage_coach");

        if (!window) {
            if (lname) g_free(lname);
            if (fname) g_free(fname);
            if (center) g_free(center);
            if (phone) g_free(phone);
            if (gender_str) g_free(gender_str);
            if (specialty) g_free(specialty);
            return;
        }

        GtkWidget *entryid = lookup_widget(window, "entryid");
        GtkWidget *entryname = lookup_widget(window, "entryname2");
        GtkWidget *entryfirst = lookup_widget(window, "entryfirstname");
        GtkWidget *entryphone = lookup_widget(window, "entryphonenumber");
        GtkWidget *spin_day = lookup_widget(window, "spinbuttonday");
        GtkWidget *spin_month = lookup_widget(window, "spinbuttonmonth");
        GtkWidget *spin_year = lookup_widget(window, "spinbuttonyear");
        GtkWidget *combo_center = lookup_widget(window, "entrycentre2");
        GtkWidget *combo_specialty = lookup_widget(window, "comboboxentry_spec");
        GtkWidget *radiobuttonman = lookup_widget(window, "radiobuttonman");
        GtkWidget *radiobuttonwomen = lookup_widget(window, "radiobuttonwomen");

        // Remplir les champs
        if (entryid) {
            gchar *id_str = g_strdup_printf("%d", id);
            gtk_entry_set_text(GTK_ENTRY(entryid), id_str);
            g_free(id_str);
        }
        
        if (entryname && lname) {
            gtk_entry_set_text(GTK_ENTRY(entryname), lname ? lname : "");
        }
        
        if (entryfirst && fname) {
            gtk_entry_set_text(GTK_ENTRY(entryfirst), fname ? fname : "");
        }
        
        if (entryphone && phone) {
            gtk_entry_set_text(GTK_ENTRY(entryphone), phone ? phone : "");
        }
        
        if (spin_day) gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_day), day);
        if (spin_month) gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_month), month);
        if (spin_year) gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_year), year);
        
        if (combo_center && center) {
            GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_center)));
            if (entry) {
                gtk_entry_set_text(entry, center ? center : "");
            }
        }
        
        if (combo_specialty && specialty) {
            GtkEntry *entry = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo_specialty)));
            if (entry) {
                gtk_entry_set_text(entry, specialty ? specialty : "");
            }
        }
        
        if (gender_str) {
            if (strcmp(gender_str, "Male") == 0) {
                gender = 1;
                if (radiobuttonman) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonman), TRUE);
                if (radiobuttonwomen) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonwomen), FALSE);
            } else {
                gender = 2;
                if (radiobuttonman) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonman), FALSE);
                if (radiobuttonwomen) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radiobuttonwomen), TRUE);
            }
        }
        
        if (lname) g_free(lname);
        if (fname) g_free(fname);
        if (center) g_free(center);
        if (phone) g_free(phone);
        if (gender_str) g_free(gender_str);
        if (specialty) g_free(specialty);
    }
    
    printf("=== DEBUG on_treeviewmanage_row_activated END ===\n");
}

// =========================================================
// REFRESH BUTTON
// =========================================================
void on_refresh_clicked(GtkButton *button, gpointer data)
{
    printf("\n=== DEBUG on_refresh_clicked ===\n");
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *tree = lookup_widget(win, "treeviewmanage");
    if (tree) refresh_tree(tree);
    
    show_popup_message(win, "Refresh", "Coach list refreshed successfully!", GTK_MESSAGE_INFO);
}

// =========================================================
// COURSE TREEVIEW ROW ACTIVATED (7 COLONNES)
// =========================================================
void on_treeview13_row_activated(GtkTreeView *treeview,
                                 GtkTreePath *path,
                                 GtkTreeViewColumn *column,
                                 gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);

    char course_id[50], course_name[50], course_type[50];
    char course_center[50], course_time[50], equipment[50], capacity[50];

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_tree_model_get(model, &iter,
                           0, course_id,
                           1, course_name,
                           2, course_type,
                           3, course_center,
                           4, course_time,
                           5, equipment,
                           6, capacity,
                           -1);

        printf("DEBUG: Course selected: %s - %s at %s (Equipment: %s, Capacity: %s)\n", 
               course_id, course_name, course_time, equipment, capacity);
    }
}

// =========================================================
// BUTTON 40 CLICKED (Load Courses) - AVEC POP-UP
// =========================================================
void on_button40_clicked(GtkButton *button, gpointer user_data)
{
    printf("\n=== DEBUG on_button40_clicked START ===\n");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));
    
    GtkWidget *treeview13 = lookup_widget(win, "treeview13");
    if (!treeview13)
    {
        printf("Error: treeview13 not found!\n");
        show_popup_message(win, "Error", "Treeview widget not found!", GTK_MESSAGE_ERROR);
        return;
    }
    
    loadCourses(GTK_TREE_VIEW(treeview13), "courses.txt");
    
    show_popup_message(win, "Success", "Courses loaded successfully!", GTK_MESSAGE_INFO);
    
    printf("=== DEBUG on_button40_clicked END ===\n");
}

// =========================================================
// BUTTON 39 CLICKED (Choose/Assign Coach to Course) - VERSION SIMPLIFIÉE
// =========================================================
void on_button39_clicked(GtkButton *button, gpointer user_data)
{
    printf("\n=== DEBUG on_button39_clicked START ===\n");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkWidget *entryid = lookup_widget(win, "entryid");
    GtkWidget *treeview13 = lookup_widget(win, "treeview13");
    GtkWidget *label201 = lookup_widget(win, "label201");

    if (!entryid || !treeview13 || !label201)
    {
        printf("Error: Some widgets not found!\n");
        show_popup_message(win, "Error", "Some widgets are missing!", GTK_MESSAGE_ERROR);
        return;
    }

    const gchar *id_text = gtk_entry_get_text(GTK_ENTRY(entryid));

    if (!id_text || strlen(id_text) == 0 || !is_valid_id(id_text))
    {
        show_popup_message(win, "Validation Error", "Invalid Coach ID! Must be numbers only (max 8 digits).", GTK_MESSAGE_WARNING);
        return;
    }

    int coach_id = atoi(id_text);
    
    printf("DEBUG: Checking if coach ID %d exists in coach.txt\n", coach_id);
    
    int coach_exists = 0;
    Coach existing_coach;
    FILE *coach_file = fopen("coach.txt", "r");
    if (coach_file)
    {
        Coach temp;
        char gender_str[10];
        char specialty[30];
        
        while (fscanf(coach_file, "%d %29s %29s %d %d %d %29s %19s %9s %29s",
                      &temp.id, temp.lastName, temp.firstName,
                      &temp.dateOfBirth.day, &temp.dateOfBirth.month, &temp.dateOfBirth.year,
                      temp.center, temp.phoneNumber, gender_str, specialty) == 10)
        {
            if (temp.id == coach_id)
            {
                existing_coach = temp;
                existing_coach.gender = (strcmp(gender_str, "Male") == 0) ? 1 : 2;
                strncpy(existing_coach.specialty, specialty, sizeof(existing_coach.specialty)-1);
                existing_coach.specialty[sizeof(existing_coach.specialty)-1] = '\0';
                coach_exists = 1;
                printf("DEBUG: Coach found: %s %s (Specialty: %s)\n", 
                       existing_coach.firstName, existing_coach.lastName, existing_coach.specialty);
                break;
            }
        }
        fclose(coach_file);
    }
    else
    {
        printf("ERROR: Cannot open coach.txt for reading\n");
    }
    
    if (!coach_exists)
    {
        gchar *error_msg = g_strdup_printf("Coach ID %d does not exist!", coach_id);
        show_popup_message(win, "Error", error_msg, GTK_MESSAGE_ERROR);
        g_free(error_msg);
        return;
    }
    
    // Vérifier si un cours est sélectionné
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview13));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (!gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        show_popup_message(win, "Warning", "Please select a course first!", GTK_MESSAGE_WARNING);
        return;
    }

    // Récupérer les informations du cours sélectionné
    gchar *course_id = NULL;
    gchar *course_name = NULL;
    gchar *course_type = NULL;
    gchar *course_center = NULL;
    gchar *course_time = NULL;
    gchar *equipment = NULL;
    gchar *capacity_text = NULL;
    
    gtk_tree_model_get(model, &iter,
                       0, &course_id,
                       1, &course_name,
                       2, &course_type,
                       3, &course_center,
                       4, &course_time,
                       5, &equipment,
                       6, &capacity_text,
                       -1);

    printf("DEBUG: Selected course - ID: '%s', Name: '%s', Capacity: '%s'\n", 
           course_id ? course_id : "NULL", course_name ? course_name : "NULL", capacity_text ? capacity_text : "NULL");

    if (course_id && strcmp(course_id, "ERROR") == 0)
    {
        show_popup_message(win, "Error", "Please load courses first!", GTK_MESSAGE_ERROR);
        
        g_free(course_id);
        g_free(course_name);
        g_free(course_type);
        g_free(course_center);
        g_free(course_time);
        g_free(equipment);
        g_free(capacity_text);
        return;
    }

    if (!course_id || strlen(course_id) == 0 || 
        !course_name || strlen(course_name) == 0 || 
        !course_center || strlen(course_center) == 0 || 
        !course_time || strlen(course_time) == 0 ||
        !capacity_text || strlen(capacity_text) == 0)
    {
        show_popup_message(win, "Error", "Invalid course data selected!", GTK_MESSAGE_ERROR);
        
        g_free(course_id);
        g_free(course_name);
        g_free(course_type);
        g_free(course_center);
        g_free(course_time);
        g_free(equipment);
        g_free(capacity_text);
        return;
    }

    int capacity = atoi(capacity_text);
    if (capacity <= 0)
    {
        show_popup_message(win, "Error", "Invalid capacity value!", GTK_MESSAGE_ERROR);
        
        g_free(course_id);
        g_free(course_name);
        g_free(course_type);
        g_free(course_center);
        g_free(course_time);
        g_free(equipment);
        g_free(capacity_text);
        return;
    }
    
    // Vérifier si le cours est déjà plein
    int assigned_coaches = count_assigned_coaches(course_id);
    
    printf("DEBUG: Course %s - Capacity: %d, Currently assigned: %d\n", 
           course_id, capacity, assigned_coaches);
    
    if (assigned_coaches >= capacity)
    {
        gchar *message = g_strdup_printf("Course is FULL! Capacity: %d, Assigned: %d", capacity, assigned_coaches);
        show_popup_message(win, "Error", message, GTK_MESSAGE_ERROR);
        g_free(message);
        
        g_free(course_id);
        g_free(course_name);
        g_free(course_type);
        g_free(course_center);
        g_free(course_time);
        g_free(equipment);
        g_free(capacity_text);
        return;
    }
    
    int day_val = existing_coach.dateOfBirth.day;
    int month_val = existing_coach.dateOfBirth.month;
    int year_val = existing_coach.dateOfBirth.year;
    
    // Nettoyer les chaînes
    gchar *clean_course_id = g_strdup(course_id ? course_id : "");
    gchar *clean_course_name = g_strdup(course_name ? course_name : "");
    gchar *clean_course_center = g_strdup(course_center ? course_center : "");
    gchar *clean_course_time = g_strdup(course_time ? course_time : "");
    
    if (clean_course_id) g_strstrip(clean_course_id);
    if (clean_course_name) g_strstrip(clean_course_name);
    if (clean_course_center) g_strstrip(clean_course_center);
    if (clean_course_time) g_strstrip(clean_course_time);
    
    char full_name[100];
    snprintf(full_name, sizeof(full_name), "%s %s", existing_coach.firstName, existing_coach.lastName);
    
    printf("DEBUG: Creating assignment - Coach: %s, Course: %s, Specialty: %s\n", 
           full_name, clean_course_name, existing_coach.specialty);
    
    // 1. SUPPRIMER LE COACH DE coach.txt
    printf("DEBUG: Removing coach ID %d from coach.txt...\n", coach_id);
    
    FILE *original_file = fopen("coach.txt", "r");
    FILE *temp_file = fopen("coach_temp.txt", "w");
    
    int coach_removed = 0;
    
    if (original_file && temp_file)
    {
        char line[256];
        while (fgets(line, sizeof(line), original_file))
        {
            int current_id;
            if (sscanf(line, "%d", &current_id) == 1)
            {
                if (current_id != coach_id)
                {
                    fputs(line, temp_file);
                }
                else
                {
                    coach_removed = 1;
                    printf("DEBUG: Coach ID %d removed from coach.txt\n", coach_id);
                }
            }
            else
            {
                fputs(line, temp_file);
            }
        }
        
        fclose(original_file);
        fclose(temp_file);
        
        if (coach_removed)
        {
            remove("coach.txt");
            rename("coach_temp.txt", "coach.txt");
            printf("DEBUG: Successfully updated coach.txt\n");
        }
        else
        {
            remove("coach_temp.txt");
            printf("DEBUG: Coach not found in coach.txt\n");
        }
    }
    else
    {
        printf("ERROR: Cannot open coach.txt or create temp file\n");
        if (original_file) fclose(original_file);
        if (temp_file) fclose(temp_file);
    }
    
    // 2. AJOUTER L'ASSIGNATION
    FILE *f = fopen("coach_assignment.txt", "a");
    if (!f)
    {
        show_popup_message(win, "Error", "Error creating assignment file!", GTK_MESSAGE_ERROR);
        
        g_free(course_id);
        g_free(course_name);
        g_free(course_type);
        g_free(course_center);
        g_free(course_time);
        g_free(equipment);
        g_free(capacity_text);
        g_free(clean_course_id);
        g_free(clean_course_name);
        g_free(clean_course_center);
        g_free(clean_course_time);
        return;
    }

    fprintf(f, "%d|%s|%s|%s|%d|%d|%d|%s|%s|%s|%s\n",
            coach_id,
            full_name,
            existing_coach.phoneNumber,
            existing_coach.specialty,
            day_val,
            month_val,
            year_val,
            clean_course_id,
            clean_course_name,
            clean_course_center,
            clean_course_time);
    
    fclose(f);
    
    printf("DEBUG: Assignment saved to coach_assignment.txt\n");
    debug_assignment_file();
    
    // 3. METTRE À JOUR LA CAPACITÉ
    int update_result = update_course_capacity(clean_course_id, -1);
    
    if (update_result) {
        // Recharger les TreeViews
        refresh_course_tree(treeview13);
        
        GtkWidget *treeview_coach = lookup_widget(win, "treeviewmanage");
        if (treeview_coach)
        {
            refresh_tree(treeview_coach);
        }
        
        int new_capacity = capacity - 1;
        int new_assigned = assigned_coaches + 1;
        int remaining = new_capacity - new_assigned;
        
        if (remaining < 0) remaining = 0;
        
        gchar *success_message = g_strdup_printf(
            "✅ Coach assigned successfully!\n\n"
            "Coach: %s\n"
            "Specialty: %s\n"
            "Course: %s\n"
            "Remaining slots: %d/%d",
            full_name, existing_coach.specialty,
            clean_course_name, remaining, new_capacity);
        
        show_popup_message(win, "Success", success_message, GTK_MESSAGE_INFO);
        g_free(success_message);
        
        // Email
        char email_body[768];
        snprintf(email_body, sizeof(email_body),
                 "ASSIGNMENT DETAILS:\n\n"
                 "COACH INFORMATION:\n"
                 "• Coach ID: %d\n"
                 "• Coach Name: %s\n"
                 "• Coach Phone: %s\n"
                 "• Coach Specialty: %s\n"
                 "• Center: %s\n"
                 "• Assignment Date: %d/%d/%d\n\n"
                 "COURSE INFORMATION:\n"
                 "• Course ID: %s\n"
                 "• Course Name: %s\n"
                 "• Course Type: %s\n"
                 "• Center: %s\n"
                 "• Time: %s\n"
                 "• Equipment: %s\n\n"
                 "CAPACITY INFORMATION:\n"
                 "• Course Capacity: %d\n"
                 "• Assigned Coaches: %d\n"
                 "• Remaining Slots: %d/%d",
                 coach_id, full_name, existing_coach.phoneNumber, existing_coach.specialty, existing_coach.center,
                 day_val, month_val, year_val,
                 clean_course_id, clean_course_name, course_type ? course_type : "N/A",
                 clean_course_center, clean_course_time, equipment ? equipment : "N/A",
                 new_capacity, new_assigned, remaining, new_capacity);
        
        char coach_email[100];
        snprintf(coach_email, sizeof(coach_email), "%s.coach@topgym.com", existing_coach.firstName);
        
        send_email_to_coach(coach_email, "Course Assignment Confirmation", email_body);
        
        printf("DEBUG: Course capacity updated: %d -> %d\n", capacity, new_capacity);
    } else {
        show_popup_message(win, "Warning", "Coach assigned but failed to update capacity!", GTK_MESSAGE_WARNING);
    }
    
    // Effacer le champ ID
    gtk_entry_set_text(GTK_ENTRY(entryid), "");
    
    // Libérer la mémoire
    g_free(course_id);
    g_free(course_name);
    g_free(course_type);
    g_free(course_center);
    g_free(course_time);
    g_free(equipment);
    g_free(capacity_text);
    g_free(clean_course_id);
    g_free(clean_course_name);
    g_free(clean_course_center);
    g_free(clean_course_time);
    
    printf("DEBUG: Assignment complete for coach %d to course %s\n", coach_id, clean_course_id);
    printf("=== DEBUG on_button39_clicked END ===\n\n");
}

// =========================================================
// CHECKBUTTON CENTER TOGGLED
// =========================================================
void on_checkbutton_center_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    gboolean active = gtk_toggle_button_get_active(togglebutton);
    printf("DEBUG: Center filter %s\n", active ? "ACTIVATED" : "DEACTIVATED");
    
    GtkWidget *win = gtk_widget_get_toplevel(GTK_WIDGET(togglebutton));
    GtkWidget *combo_center = lookup_widget(win, "entrycentre1");
    
    if (combo_center) {
        gtk_widget_set_sensitive(combo_center, active);
    }
}
