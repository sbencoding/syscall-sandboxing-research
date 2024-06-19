from common import ProgAnalysis, input_case
import tempfile
import shutil

# https://www.sqlite.org/lang.html
class AnlSqlite(ProgAnalysis):
    def __init__(self):
        super(AnlSqlite, self).__init__('/usr/bin/sqlite3')
        self.tmp_dir = tempfile.mkdtemp()
        self.tmp_db = self.tmp_dir + '/test.db'

    def exec_sql(self, statement):
        return self.exec_with_args([self.tmp_db, statement])

    @input_case
    def case_create_db(self):
        self.exec_sql('')

    @input_case
    def case_create_table(self):
        sql = '''CREATE TABLE test1 (
            id INTEGER PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            counter INTEGER DEFAULT 0,
            value DOUBLE DEFAULT 10.0,
            data BLOB
        )'''
        self.exec_sql(sql)

    @input_case
    def case_insert_data(self):
        self.exec_sql("INSERT INTO test1 (id, name, data) VALUES (0, 'ghost', x'1337')")

    @input_case
    def case_select_data(self):
        self.exec_sql("SELECT id, name, HEX(data) FROM test1")

    @input_case
    def case_insert_multiple(self):
        self.exec_sql("INSERT INTO test1 (id, name, data) VALUES (1, 'test', x'1336'), (2, 'root', x'1338')")

    @input_case
    def case_select_data_where(self):
        self.exec_sql("SELECT id, name, HEX(data) FROM test1 WHERE id=1")

    @input_case
    def case_select_data_like(self):
        self.exec_sql("SELECT id, name, HEX(data) FROM test1 WHERE name LIKE '%st'")

    @input_case
    def case_update_data(self):
        self.exec_sql("UPDATE test1 SET counter=1")

    @input_case
    def case_update_data_where(self):
        self.exec_sql("UPDATE test1 SET value=11.1 WHERE id=0")

    @input_case
    def case_update_transactional(self):
        self.exec_sql("BEGIN TRANSACTION; UPDATE test1 SET value=11.1 WHERE id=0; UPDATE test1 SET value=12 WHERE id=2; COMMIT")

    @input_case
    def case_update_transactional_rollback(self):
        self.exec_sql("BEGIN TRANSACTION; UPDATE test1 SET value=11.1 WHERE id=0; UPDATE test1 SET value=12 WHERE id=2; ROLLBACK")

    @input_case
    def case_get_date(self):
        self.exec_sql("SELECT date()")

    @input_case
    def case_get_time(self):
        self.exec_sql("SELECT unixepoch('subsec')")

    @input_case
    def case_create_view(self):
        self.exec_sql("CREATE VIEW testview1 AS SELECT name from test1 WHERE value <= 15.6")

    @input_case
    def case_query_view(self):
        self.exec_sql("SELECT * FROM testview1")

    @input_case
    def case_remove_view(self):
        self.exec_sql("DROP VIEW testview1")

    @input_case
    def case_query_vtable(self):
        self.exec_sql("SELECT * FROM dbstat")

    @input_case
    def case_analyze(self):
        self.exec_sql("ANALYZE test1")

    @input_case
    def case_create_index(self):
        self.exec_sql("CREATE INDEX testindex1 ON test1 (value)")

    @input_case
    def case_query_with_index(self):
        self.exec_sql("SELECT name, value FROM test1 INDEXED BY testindex1 WHERE value <= 10.9")

    @input_case
    def case_drop_index(self):
        self.exec_sql("DROP INDEX testindex1")

    @input_case
    def case_delete_from(self):
        self.exec_sql("DELETE FROM test1 WHERE id=0")

    @input_case
    def case_drop_table(self):
        self.exec_sql("DROP TABLE test1")

    def stats(self):
        print('Database located at:', self.tmp_db)
        ProgAnalysis.stats(self)

class AnlSqliteMP(AnlSqlite):
    def __init__(self):
        super(AnlSqliteMP, self).__init__()
        # `shell_exec` function
        self.enable_multi_phase([0x0000000000021df0])

if __name__ == '__main__':
    asp = AnlSqliteMP()
    asp.run_all_cases()
    asp.stats()
