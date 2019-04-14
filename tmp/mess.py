from time import sleep


class Foo:
    def __init__(self, num):
        self.num = num
    def __str__(self):
        return str(self.num)

if __name__ == '__main__':
    dicc = {'quim':Foo(5), 'ian':Foo(3)}
    while True:  # TODO:Change so it breaks when quit prot
        for item in dicc:
            dicc[item].num -= 1
        sleep(1/2)
        print [dicc[foo].num for foo in dicc]