from common import ProgAnalysis

class AnlLs(ProgAnalysis):
    def __init__(self):
        super(AnlLs, self).__init__('/bin/ls')

    def case_simple(self):
        self.exec()

    def case_more_output(self):
        self.exec_with_args(['-lah'])

    def case_no_access(self):
        self.exec_with_args(['-lah', '/root'])

    def case_dev(self):
        self.exec_with_args(['-lah', '/dev'])

    def case_deref(self):
        self.exec_with_args(['-Llah', '/dev'])

    def case_get_inode(self):
        self.exec_with_args(['-lahi', '/dev'])

    def case_get_blocksize(self):
        self.exec_with_args(['-lahs', '/dev'])

    def case_sort_by_time(self):
        self.exec_with_args(['-laht', '/dev'])

if __name__ == '__main__':
    asp = AnlLs()
    asp.run_all_cases()
    asp.stats()
