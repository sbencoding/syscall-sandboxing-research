from common import ProgAnalysis, input_case

class AnlLs(ProgAnalysis):
    def __init__(self):
        super(AnlLs, self).__init__('/bin/ls')

    @input_case
    def case_simple(self):
        self.exec()

    @input_case
    def case_more_output(self):
        self.exec_with_args(['-lah'])

    @input_case
    def case_no_access(self):
        self.exec_with_args(['-lah', '/root'])

    @input_case
    def case_dev(self):
        self.exec_with_args(['-lah', '/dev'])

    @input_case
    def case_deref(self):
        self.exec_with_args(['-Llah', '/dev'])

    @input_case
    def case_get_inode(self):
        self.exec_with_args(['-lahi', '/dev'])

    @input_case
    def case_get_blocksize(self):
        self.exec_with_args(['-lahs', '/dev'])

    @input_case
    def case_sort_by_time(self):
        self.exec_with_args(['-laht', '/dev'])

class AnlLsMP(AnlLs):
    def __init__(self):
        super(AnlLsMP, self).__init__()
        # call to `clear_files` from `main`
        self.enable_multi_phase([0x0000000000004b55])

if __name__ == '__main__':
    asp = AnlLsMP()
    asp.run_all_cases()
    asp.stats()
