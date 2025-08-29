from nicegui import ui
import asyncio

from bmslib.jikong import JKBt

monitor_task = None

soc = 0.00
current = 0.000

async def monitor_bms():
    mac_address = 'C8:47:80:23:4F:95'  # Replace with your BMS MAC address

    bms = JKBt(mac_address, name='jk', verbose_log=False)
    async with bms:
        while True:
            try:
                s = await bms.fetch(wait=True)
                global soc, current
                soc = s.soc
                print(f"SOC#: {soc}")
                current = s.current
                print(f"SOC: {s.soc:.2f} Current: {s.current:.3f} Voltage: {s.voltage:.3f} Temp: {s.temperatures} I_bal: {s.balance_current} Voltages: {await bms.fetch_voltages()}")
                await asyncio.sleep(1)  # Add this line to prevent a tight loop
            except KeyboardInterrupt:
                break

async def start_monitoring():
    global monitor_task
    if monitor_task is None or monitor_task.done():
        monitor_task = asyncio.create_task(monitor_bms())


async def stop_monitoring():
    global monitor_task
    if monitor_task and not monitor_task.done():
        monitor_task.cancel()
        try:
            await monitor_task
        except asyncio.CancelledError:
            print("Monitoring task cancelled.")

#class Value:
#    def __init__(self):
#        self.number = None
#
#soc = Value()
#current = Value()

@ui.page('/')
async def home_page():
    """
    Main page of the GUI.
    """

    with ui.row().classes('w-full'):
        with ui.tabs().classes('items-start') as tabs:
            home_tab = ui.tab('Home', icon='home')
            settings_tab = ui.tab('Settings', icon='settings')
        with ui.tab_panels(tabs, value=home_tab).classes('w-full'):
            with ui.tab_panel(home_tab):
                ui.label('Welcome to the VoBLE BMS Monitor!').classes('text-2xl m-4')
                ui.label('Use the buttons in the Settings tab to start or stop monitoring the BMS.').classes('m-4')
                with ui.column().classes('items-center w-[300px]'):
                    global soc
                    gauge_soc = 0
                    
                    gauge_options = {
                        'title': {'text': ''},
                        'chart': {'type': 'solidgauge', 'backgroundColor': 'transparent'},  # Makes chart background transparent
                        'credits': {'enabled': False},  # Removes the highcharts.com watermark
                        'pane': {
                            'center': ['50%', '50%'],
                            'size': '200px',
                            'startAngle': -150,
                            'endAngle': 150,
                            'background': {
                                'backgroundColor': '#333',
                                'innerRadius': '60%',
                                'outerRadius': '100%',
                                'shape': 'arc'
                            }
                        },
                        'yAxis': {'min': 0.00,
                                  'max': 100.00,
                                  'lineWidth': 0,
                                  'minorTickLength': 0,
                                  'tickLength': 0,
                                  'labels': {'enabled': False},
                        },
                        'series': [{'data': [gauge_soc],
                                    'dataLabels': {
                                        'enabled': True,
                                        'borderWidth': 0,
                                        'backgroundColor': 'transparent',
                                        'style': {
                                            'fontSize': '32px',
                                            'fontWeight': 'bold',
                                            'color': '#FFF',
                                            'textOutline': 'none'
                                        }
                                    }},]
                        }
                    
                    def update_gauge():
                        global soc
                        nonlocal gauge_soc
                        # This prevents the guage from animating from 0 to current value every time the SOC is read.
                        if gauge_soc != soc:
                            gauge_soc = soc
                            gauge_options['series'][0]['data'][0] = gauge_soc
                            soc_gauge.update()

                    ui.timer(1.0, update_gauge)
                    ui.label('SOC').classes('text-xl font-bold mt-2 m-4') # TODO: add dynamic battery name to the SOC label
                    soc_gauge = ui.highchart(gauge_options, extras=['solid-gauge']).classes('w-full h-64')
        with ui.tab_panels(tabs, value=settings_tab).classes('w-full'):
            with ui.tab_panel(settings_tab):
                with ui.row():
                    ui.button(icon='play_arrow', on_click=start_monitoring).classes('m-2')
                    ui.button(icon='stop', on_click=stop_monitoring).classes('m-2')



ui.run(title='VoBLE', reload=True, host='0.0.0.0', port=4850, dark=True)