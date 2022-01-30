#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define ID_FILE_NAME "ID/ID_MAX.bin"                                        // the path to file where the last id saves
#define TEXT_DATABASE_NAME "PERSON_DAT/PERSON.dat"                          // the path to database
#define TEXT_DATABASE_NAME_NEW "PERSON_DAT/PERSON_NEW.dat"                  // the path to new database for shifting database

#define PRINT_FORMAT "%40lld %50s %50s %40lld\n"                            // format for showing all database
#define WRITING_FORMAT "%40lld %250s %250s %40lld\n"                        // format for writing to database, reading from database
#define WRITING_FORMAT_LEN 584                                              // the len of WRITNG FORMAT
#define BUFFER_SIZE 250                                                     // max buffer size for name and surname

#define MAX_PERSON_IN_DYNAMIC_ARRAY 50                                      // max count of people in dynamic array

typedef long long _id_len;                                                  // variable for id
typedef unsigned long long _pesel_len;                                      // variable for PESEL
typedef char _byte;                                                         // variable for byte
typedef long _position;                                                     // variable for position in file

_id_len id = 0;                                                             // global variable for ID

struct person                                                               // person structure
{
    _id_len id;
    char name[BUFFER_SIZE];
    char surname[BUFFER_SIZE];
    _pesel_len pesel;
};

struct person_position                                                      // person structure with position in dynamic array
{
    struct person person;
    _byte pos;
};

_id_len get_current_id()                                                    // get current id from ID_MAX.bin
{                                                                           // if there is not any file with ID_MAX.bin name
    _id_len id;                                                             // create new and read the last line in PERSON.dat
    FILE *from = fopen(ID_FILE_NAME, "rb");                                 // if reading was unsuccessful, create PERSON.dat and return 0
    struct person per;                                                      // else return the last written id
    if (!from)
    {
        mkdir("ID", 0777);
        creat(ID_FILE_NAME, 0777);
        from = fopen(TEXT_DATABASE_NAME, "r");
        if (!from)
        {
            mkdir("PERSON_DAT", 0777);
            creat(TEXT_DATABASE_NAME, 0777);
            return 0;
        }
        if (fseek(from, -(WRITING_FORMAT_LEN + 1), SEEK_END) == -1)
        {
            fclose(from);
            return 0;
        }
        fscanf(from, WRITING_FORMAT, &per.id, per.name, per.surname, &per.pesel);
        fclose(from);
        return ++per.id;
    }
    fread(&id, sizeof(_id_len), 1, from);
    fclose(from);
    return id;
}

void save_id(_id_len id)                                                    // save id before exiting from the program
{   
    FILE *to = fopen(ID_FILE_NAME, "wb");
    fwrite(&id, sizeof(_id_len), 1, to);
    fclose(to);
}

long get_file_len(const char *path)                                         // the function for getting the size of file
{
    FILE *from = fopen(path, "r");
    if (from == NULL)
    {
        return -1;
    }
    fseek(from, 0, SEEK_END);
    int len = ftell(from);
    fclose(from);
    return len;
}

void append_person(struct person *per)                                      // write the person in the last position of the DB, make the increment of ID
{
    FILE *to = fopen(TEXT_DATABASE_NAME, "a");
    if (!to)
    {
        creat(TEXT_DATABASE_NAME, 0777);
        to = fopen(TEXT_DATABASE_NAME, "a");
    }
    fprintf(to, WRITING_FORMAT, id, per->name, per->surname, per->pesel);
    id++;
    fclose(to);
}

void person_copy(const struct person *from, struct person *to)              // the function for copy person structure in other person structure
{
    to->id = from->id;
    strcpy(to->name, from->name);
    strcpy(to->surname, from->surname);
    to->pesel = from->pesel;
}

void sort(struct person_position *array, int len)                           // sort the array of struct person_position saving indexes of begining condition
{
    int check = 1;
    struct person_position buf;
    while (check)
    {
        check = 0;
        for (int i = 0; i < len - 1; i++)
        {
            if (array[i].person.pesel > array[i + 1].person.pesel)
            {
                person_copy(&array[i + 1].person, &buf.person);
                buf.pos = array[i + 1].pos;
                person_copy(&array[i].person, &array[i + 1].person);
                array[i + 1].pos = array[i].pos;
                person_copy(&buf.person, &array[i].person);
                array[i].pos = buf.pos;
                check++;
            }
        }
    }
}

int search_bin(_pesel_len pesel, const struct person_position *arr, int len, int low, int high)
{                                                                           // search the person by PESEL in dynamic array,
    int mid = (low + high) / 2;                                             // return the position in the dynamic array, else -1
    if (low < 0 || high < 0)
        return -1;
    if (low > high)
        return -1;

    if (pesel > arr[mid].person.pesel)
        return search_bin(pesel, arr, len, mid + 1, high);
    else if (pesel < arr[mid].person.pesel)
        return search_bin(pesel, arr, len, low, mid - 1);
    else if (arr[mid].person.pesel == pesel)
        return mid;
    return -1;
}

_position get_by_pesel(_pesel_len pesel, struct person *per)
{                                                                           // search the person in the DB, return position in the file, else if not found, -1
    long file_len = get_file_len(TEXT_DATABASE_NAME), count_of_people = file_len / (WRITING_FORMAT_LEN + 1), position = 0;
    long iterations = count_of_people <= MAX_PERSON_IN_DYNAMIC_ARRAY ? 1 : (count_of_people / MAX_PERSON_IN_DYNAMIC_ARRAY) + 1;
    long person_in_file_left = count_of_people;
    long i;
    int j, count_of_people_in_array, position_in_array = 0;
    struct person_position *people_arr;
    FILE *from = fopen(TEXT_DATABASE_NAME, "r");
    if (!from)
        perror("error #TEXT_DATABASE_NAME_OPENING_GET_BY_PESEL");

    for (i = 0; i < iterations; i++)
    {
        if (person_in_file_left < MAX_PERSON_IN_DYNAMIC_ARRAY)
            count_of_people_in_array = person_in_file_left;
        else
        {
            count_of_people_in_array = MAX_PERSON_IN_DYNAMIC_ARRAY;
            person_in_file_left -= MAX_PERSON_IN_DYNAMIC_ARRAY;
        }
        people_arr = malloc(sizeof(struct person_position) * count_of_people_in_array);
        for (j = 0; (j < count_of_people_in_array) && (fscanf(from, WRITING_FORMAT, &people_arr[j].person.id, people_arr[j].person.name, people_arr[j].person.surname, &people_arr[j].person.pesel) != EOF); j++)
            people_arr[j].pos = j;
        sort(people_arr, count_of_people_in_array);
        position_in_array = search_bin(pesel, people_arr, count_of_people_in_array, 0, count_of_people_in_array - 1);
        if (position_in_array != -1)
        {
            position += people_arr[position_in_array].pos;
            person_copy(&people_arr[position_in_array].person, per);
            free(people_arr);
            fclose(from);
            return position;
        }
        free(people_arr);
        position += count_of_people_in_array;
    }
    fclose(from);
    return -1;
}

_position delete_by_pesel(_pesel_len pesel)                                 // delete the person by his PESEL,
{                                                                           // if there is not any person with that PESEL, return -1
    struct person per;
    int stat = get_by_pesel(pesel, &per);
    if (stat == -1)
        return -1;
    FILE *where = fopen(TEXT_DATABASE_NAME, "r+");
    if (!where)
        perror("error #TEXT_DATABASE_NAME_OPENING_DELETE_BY_PESEL");

    fseek(where, WRITING_FORMAT_LEN * stat, SEEK_SET);
    fprintf(where, WRITING_FORMAT, (_id_len)-1, "NULL", "NULL", (_pesel_len)0);
    fclose(where);
    return stat;
}

_position edit_by_pesel(_pesel_len pesel, const struct person *per)         // edit the person by his PESEL
{                                                                           // if there is not any person with that PESEL, return -1
    struct person temp;
    int stat = get_by_pesel(pesel, &temp);
    if (!stat)
        return -1;
    FILE *where = fopen(TEXT_DATABASE_NAME, "r+");
    if (!where)
        perror("error #TEXT_DATABASE_NAME_OPENING_EDIT_BY_PESEL");

    fseek(where, WRITING_FORMAT_LEN * stat, SEEK_SET);
    fprintf(where, WRITING_FORMAT, temp.id, per->name, per->surname, per->pesel);
    fclose(where);
    return stat;
}

void shift_database()                                                       // make the new copy of DB skipping deleted people
{
    struct person per;
    FILE *from = fopen(TEXT_DATABASE_NAME, "r");
    if (!from)
        perror("error #TEXT_DATABASE_NAME_OPENING_SHIFT_DATABASE");
    FILE *to = fopen(TEXT_DATABASE_NAME_NEW, "w+");
    if (!to)
        perror("error #TEXT_DATABASE_NAME_NEW_OPENING_SHIFT_DATABASE");
    while (fscanf(from, WRITING_FORMAT, &per.id, per.name, per.surname, &per.pesel) != EOF)
    {
        if (per.id == -1)
            continue;
        fprintf(to, WRITING_FORMAT, per.id, per.name, per.surname, per.pesel);
    }
    fclose(from);
    fclose(to);
    if (remove(TEXT_DATABASE_NAME) == -1)
        perror("error #TEXT_DATABASE_REMOVE_SHIFT_DATABASE");
    if (rename(TEXT_DATABASE_NAME_NEW, TEXT_DATABASE_NAME) == -1)
        perror("error #TEXT_DATABASE_RENAME_SHIFT_DATABASE");
}
// START CODE OF MENU OF THE PROGRAM
enum choice
{
    app_person = 1,
    delete_person,
    edit_person,
    print_table,
    search_person,
    shift_db,
    exit_programm,
};

void menu_append_person(struct person *per)
{
    printf("Write a name: ");
    scanf("%s", per->name);
    printf("Write a surname: ");
    scanf("%s", per->surname);
    printf("Write a PESEL: ");
    scanf("%lld", &per->pesel);
    append_person(per);
}

void menu_delete_person(struct person *per)
{
    _pesel_len pes;
    printf("Write a PESEL person who you want to delete: ");
    scanf("%lld", &pes);
    if (delete_by_pesel(pes) == -1)
        printf("Person with PESEL %lld not found...\n", pes);
    else
        printf("Person with PESEL %lld deleted...\n", pes);
}

void menu_edit_person(struct person *per)
{
    _pesel_len pes;
    printf("Write a PESEL person who you want to edit: ");
    scanf("%lld", &pes);
    printf("Write a name: ");
    scanf("%s", per->name);
    printf("Write a surname: ");
    scanf("%s", per->surname);
    printf("Write a PESEL: ");
    scanf("%lld", &per->pesel);
    if (edit_by_pesel(pes, per) == -1)
        printf("Person with PESEL %lld not found...\n", pes);
    else
        printf("Person with PESEL %lld edited...\n", pes);
}

void menu_print_table()
{
    FILE *from = fopen(TEXT_DATABASE_NAME, "r");
    struct person per;
    if (!from)
    {
        printf("file not found...\n");
        return;
    }
    while (fscanf(from, WRITING_FORMAT, &per.id, per.name, per.surname, &per.pesel) != EOF)
        if (per.id != -1)
            printf(PRINT_FORMAT, per.id, per.name, per.surname, per.pesel);
    fclose(from);
}

void menu_search_person(struct person *per)
{
    _pesel_len pes;
    printf("Write a PESEL person who you want to delete: ");
    scanf("%lld", &pes);
    if (-1 == get_by_pesel(pes, per))
    {
        printf("Person not found...\n");
        return;
    }
    printf("PERSON ->\n\tNAME -> %s\n\tSURNAME -> %s\n\tPESEL -> %lld\n", per->name, per->surname, per->pesel);
}

void menu_shift_db()
{
    printf("start shifting database...\n");
    shift_database();
    printf("database shifted...\n");
}

void menu_exit_program()
{
    save_id(id);
}

int go_programm(struct person *per)
{
    id = get_current_id();
    for (;;)
    {
        char num;
        printf("MENU:\n\t1 -> append person in database\n\t2 -> delete person from the database by PESEL\n\t3 -> edit person in the database by PESEL\n\t4 -> print all database\n\t5 -> find a person by PESEL in database\n\t6 -> shift database\n\t7 -> exit from the program\n");
        printf("Write a number what do you want to do: ");
        scanf("%hhd", &num);
        switch (num)
        {
        case app_person:
            menu_append_person(per);
            break;
        case delete_person:
            menu_delete_person(per);
            break;
        case edit_person:
            menu_edit_person(per);
            break;
        case print_table:
            menu_print_table();
            break;
        case search_person:
            menu_search_person(per);
            break;
        case shift_db:
            menu_shift_db();
            break;
        case exit_programm:
            menu_exit_program();
            return 0;
            break;
        default:
            printf("Unknown command...\n");
            break;
        }
    }
}
// END
int main()
{
    struct person per;
    return go_programm(&per);
}
