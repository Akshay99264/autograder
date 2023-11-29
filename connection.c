#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

int main() {
    // Initialize a PostgreSQL connection object
    PGconn *conn = PQconnectdb("dbname=autograder user=postgres password=Akshay#123 host=localhost");
    PGresult *res;
    // Check for a successful connection
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    const char *createTableSQL = "CREATE TABLE IF NOT EXISTS mytable (id serial primary key, name text)";
    res = PQexec(conn, createTableSQL);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Table creation failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        exit(1);
    }
    // Your database operations go here...

    // Close the connection when done
    PQfinish(conn);

    return 0;
}
