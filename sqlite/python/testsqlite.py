import sqlite3
import os


os.remove("core_0_functracer.db")
conn = sqlite3.connect("core_0_functracer.db")

cursor = conn.cursor()

sql = '''CREATE TABLE TRACERECORD(
    IC BIGINT NOT NULL,
    NAME CHAR(40) NOT NULL,
    OP CHAR(1) NOT NULL
)'''

cursor.execute(sql)

datalist = [['syncAREinGICD', 50, 'c'],
            ['syncAREinGICD', 54, 'r'],
            ['helloworld', 74, 'c'],
            ['main', 90, 'c'],
            ['main', 123, 'r'],
            ['helloworld', 174, 'r']]

for data in datalist:
    sql = '''INSERT INTO TRACERECORD(NAME, IC, OP) VALUES ('%s', %d, '%s')''' % (data[0], data[1], data[2])
    cursor.execute(sql)
    
cursor.execute('''SELECT * FROM TRACERECORD''')
result = cursor.fetchall()
print(result)

conn.commit()
conn.close()