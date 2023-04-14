import sqlite3
conn = sqlite3.connect('test.db')

cursor = conn.cursor()

sql = '''SELECT * FROM COMPANY'''
cursor.execute(sql)

result = cursor.fetchall()
print(result)

conn.commit()
conn.close()