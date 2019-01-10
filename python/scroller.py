import datetime


class Scroller(object):
    def __init__(self, min_value, max_value, speed=None):
        self.min_value = min_value
        self.max_value = max_value
        self._value = min_value
        self._scroll_speed = (max_value - min_value) / 10 if speed is None else speed
        self._scroll_start = None
        self._scroll_direction = 0

    def _value_at(self, t):
        if self._scroll_direction == 0:
            return self._value
        dv = (t - self._scroll_start).total_seconds() * self._scroll_speed * self._scroll_direction
        return min(max(self._value + dv, self.min_value), self.max_value)

    def _finish_scroll(self, t):
        if self._scroll_direction != 0:
            self._value = self._value_at(t)

    def scroll_down(self, scroll=True):
        self.scroll(-1, scroll)

    def scroll_up(self, scroll=True):
        self.scroll(1, scroll)

    def scroll(self, direction, scroll=True):
        if scroll:
            now = datetime.datetime.utcnow()
            self._finish_scroll(now)
            self._scroll_start = now
            self._scroll_direction = direction
        else:
            self.scroll_stop()

    def scroll_stop(self):
        now = datetime.datetime.utcnow()
        self._finish_scroll(now)
        self._scroll_start = None
        self._scroll_direction = 0

    def get_value(self):
        if self._scroll_direction == 0:
            return self._value
        return self._value_at(datetime.datetime.utcnow())

    def set_value(self, value):
        self._value = value
        self._scroll_start = datetime.datetime.utcnow()

    value = property(get_value, set_value)

    def get_speed(self):
        return self._scroll_speed

    def set_speed(self, speed):
        now = datetime.datetime.utcnow()
        self._value = self._value_at(now)
        self._scroll_start = now
        self._scroll_speed = speed

    speed = property(get_speed, set_speed)
