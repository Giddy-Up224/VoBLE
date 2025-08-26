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
                with ui.row():
                    global soc
                    
                    gauge_options = {
                        'title': {'text': 'State of Charge (SOC)'},
                        'chart': {'type': 'solidgauge'},
                        'yAxis': {'min': 0.00, 'max': 100.00,},
                        'series': [{'data': [soc]},]
                        }
                    
                    def update_gauge():
                        global soc
                        gauge_options['series'][0]['data'][0] = soc
                        soc_gauge.update()

                    ui.timer(1.0, update_gauge)
                    soc_gauge = ui.highchart(gauge_options, extras=['solid-gauge']).classes('w-full h-64')
        with ui.tab_panels(tabs, value=settings_tab).classes('w-full'):
            with ui.tab_panel(settings_tab):
                with ui.row():
                    ui.button(icon='play_arrow', on_click=start_monitoring).classes('m-2')
                    ui.button(icon='stop', on_click=stop_monitoring).classes('m-2')



ui.run(title='VoBLE', reload=True, host='0.0.0.0', port=4850, dark=True)