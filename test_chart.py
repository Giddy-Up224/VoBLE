from nicegui import ui
from random import random

options = {
    'title': False,
    'chart': {'type': 'bar'},
    'xAxis': {'categories': ['A', 'B']},
    'series': [
        {'name': 'Alpha', 'data': [0.1, 0.2]},
        {'name': 'Beta', 'data': [0.3, 0.4]},
    ],
}

chart = ui.highchart(options).classes('w-full h-64')

def update():
    options['series'][0]['data'][0] = random()
    options['series'][1]['data'][1] = random()
    chart.update()

ui.button('Update', on_click=update)

ui.run()