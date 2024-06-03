from common import ProgAnalysis, input_case

class AnlRedis(ProgAnalysis):
    def __init__(self):
        super(AnlRedis, self).__init__('/usr/bin/redis-server')

    @input_case
    def case_bigtest(self):
        self.exec_with_external('/usr/bin/redis-benchmark', ['-n', '10'])

if __name__ == '__main__':
    asp = AnlRedis()
    asp.run_all_cases()
    asp.stats()
