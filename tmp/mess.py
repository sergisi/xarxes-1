import threading
from time import sleep
dicc = 5


def foo():
    global dicc
    dicc = {'quim':'gay', 'ian':'petita'}

if __name__ == '__main__':
    t1 = threading.Thread(target=foo)
    t1.start()
    t1.join()
    print dicc
