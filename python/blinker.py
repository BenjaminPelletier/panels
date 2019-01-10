import datetime


class Blinker(object):
    def __init__(self, actions, stop_action):
        self._actions = actions
        self._stop_action = stop_action
        self._next_action = 0
        self._next_change = None
        self._period = None

    def blink(self, frequency):
        self._period = datetime.timedelta(seconds=1 / frequency / len(self._actions))
        if self._next_change is None or self._next_change > datetime.datetime.utcnow() + self._period:
            self._next_change = datetime.datetime.utcnow() + self._period

    def stop(self):
        if self._next_change is not None:
            self._next_change = None
            self._actions[self._stop_action]()

    def update(self):
        if self._next_change is None:
            return
        if datetime.datetime.utcnow() >= self._next_change:
            self._actions[self._next_action]()
            self._next_action = (self._next_action + 1) % len(self._actions)
            self._next_change += self._period
