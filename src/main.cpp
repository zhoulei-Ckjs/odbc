#include <iostream>
#include <sql.h>
#include <sqlext.h>
#include <unistd.h>
#include <string.h>

void HandleError(SQLHANDLE handle, SQLSMALLINT handleType, const std::string& msg)
{
    SQLCHAR sqlState[6];
    SQLINTEGER nativeError;
    SQLCHAR message[256];
    SQLSMALLINT msgLen;

    std::cerr << "Error: " << msg << std::endl;
    // 循环获取所有诊断记录
    for (SQLSMALLINT i = 1; ; ++i)
    {
        SQLRETURN ret = SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError,
                                      message, sizeof(message), &msgLen);
        if (ret == SQL_NO_DATA)
            break;
        if (SQL_SUCCEEDED(ret))
        {
            std::cerr << "  SQLState: " << sqlState
                      << ", Native Error: " << nativeError
                      << ", Message: " << message << std::endl;
        }
    }
}

bool InitAndConnect(SQLHENV& hEnv, SQLHDBC& hDbc,
                    const std::string& dsn, const std::string& user, const std::string& pwd)
{
    // 1. 分配环境句柄
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) != SQL_SUCCESS)
    {
        std::cerr << "Failed to allocate environment handle." << std::endl;
        return false;
    }
    // 2. 声明 ODBC 3.0 版本
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    // 3. 分配连接句柄
    if (SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc) != SQL_SUCCESS)
    {
        std::cerr << "Failed to allocate connection handle." << std::endl;
        return false;
    }
    // 4. 连接数据库
    SQLRETURN ret = SQLConnect(hDbc,
                               (SQLCHAR*)dsn.c_str(), SQL_NTS,
                               (SQLCHAR*)user.c_str(), SQL_NTS,
                               (SQLCHAR*)pwd.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret))
    {
        HandleError(hDbc, SQL_HANDLE_DBC, "Connection failed");
        return false;
    }
    std::cout << "Connected to database successfully." << std::endl;
    return true;
}

void QueryWithDirect(SQLHDBC& hDbc, const std::string& sql)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT; // 一条 sql 语句。
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    if (SQL_SUCCEEDED(ret))
    {
        // 假设查询返回 id (int) 和 name (varchar)
        SQLINTEGER id;
        SQLCHAR name[64];
        SQLLEN id_ind, name_ind;

        // 绑定结果列
        SQLBindCol(hStmt, 1, SQL_C_LONG, &id, 0, &id_ind);
        SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &name_ind);

        // 循环获取每一行
        while (SQLFetch(hStmt) == SQL_SUCCESS)
        {
            std::cout << "ID: " << id << ", Name: " << name << std::endl;
        }
    }
    else
    {
        HandleError(hStmt, SQL_HANDLE_STMT, "Query failed");
    }
    if (hStmt != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void InsertData(SQLHDBC& hDbc,
                int id, const std::string& name, double salary)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT; // 一条 sql 语句。
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    const char* sql = "INSERT INTO employees (id, name, salary) VALUES (?, ?, ?)";
    SQLPrepare(hStmt, (SQLCHAR*)sql, SQL_NTS);

    // 绑定三个参数
    SQLINTEGER  idVal = id;
    SQLCHAR     nameVal[64];
    strcpy((char*)nameVal, name.c_str());
    SQLDOUBLE   salaryVal = salary;
    SQLLEN      idInd = 0, nameInd = strlen((char*)nameVal), salaryInd = 0;

    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,   SQL_INTEGER, 0, 0, &idVal,     0, &idInd);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR,   SQL_VARCHAR, 64, 0, nameVal,    0, &nameInd);
    SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_DOUBLE, SQL_DOUBLE,  0, 0, &salaryVal, 0, &salaryInd);

    SQLRETURN ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret))
    {
        std::cout << "Insert successful. Rows affected: " << std::endl;
    }
    else
    {
        HandleError(hStmt, SQL_HANDLE_STMT, "Insert failed");
    }
    if (hStmt != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void DeleteData(SQLHDBC& hDbc,
                int id)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT; // 一条 sql 语句。
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    const char* sql = "DELETE FROM employees WHERE id = ?";
    SQLPrepare(hStmt, (SQLCHAR*)sql, SQL_NTS);

    SQLINTEGER idVal = id;
    SQLLEN idInd = 0;
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &idVal, 0, &idInd);

    SQLRETURN ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret))
    {
        std::cout << "Delete successful." << std::endl;
    }
    else
    {
        HandleError(hStmt, SQL_HANDLE_STMT, "Delete failed");
    }
    if (hStmt != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void UseDatabase(SQLHDBC& hDbc,
                 const std::string& dbName)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT; // 一条 sql 语句。
    // 方案一：直接执行 USE 语句 (注意 SQL 注入风险，生产环境需校验 dbName)
    std::string sql = "USE " + dbName;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    if (SQL_SUCCEEDED(ret))
    {
        std::cout << "Switched to database: " << dbName << std::endl;
    }
    else
    {
        HandleError(hStmt, SQL_HANDLE_STMT, "USE database failed");
    }
    if (hStmt != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

bool CreateTable(SQLHDBC& hDbc,
                 const std::string& sql)
{
    SQLHSTMT hStmt = SQL_NULL_HSTMT; // 一条 sql 语句。
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLRETURN ret = SQLExecDirect(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);

    bool success = SQL_SUCCEEDED(ret);
    if (success)
    {
        std::cout << "Table created successfully." << std::endl;
    }
    else
    {
        HandleError(hStmt, SQL_HANDLE_STMT, "Create table failed");
    }

    if (hStmt != SQL_NULL_HSTMT)
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return success;
}

int main()
{
    SQLHENV env = SQL_NULL_HENV;
    SQLHDBC dbc = SQL_NULL_HDBC;    // 一条数据库连接

    InitAndConnect(env, dbc, "DM8_TEST", "SYSDBA", "Ckjs220807");

    CreateTable(dbc, "CREATE TABLE employees (id INT PRIMARY KEY, name VARCHAR(64))");

    // 释放句柄
    if (dbc  != SQL_NULL_HDBC)  { SQLDisconnect(dbc); SQLFreeHandle(SQL_HANDLE_DBC, dbc); }
    if (env  != SQL_NULL_HENV)  SQLFreeHandle(SQL_HANDLE_ENV, env);
    return 0;
}