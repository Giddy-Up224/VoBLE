from nicegui import ui
import asyncio

from bmslib.jikong import JKBt

monitor_task = None

async def monitor_bms():
    mac_address = 'C8:47:80:23:4F:95'  # Replace with your BMS MAC address

    bms = JKBt(mac_address, name='jk', verbose_log=False)
    async with bms:
        while True:
            try:
                s = await bms.fetch(wait=True)
                print(f"SOC: {repr(s.soc)} Current: {s.current:.3f} Voltage: {s.voltage:.3f} Temp: {s.temperatures} I_bal: {s.balance_current} Voltages: {await bms.fetch_voltages()}")
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


@ui.page('/')
async def home_page():
    """
    Home page of the GUI.
    """
    with ui.row().classes('w-full'):
        with ui.tabs().classes('items-start') as tabs:
            home_tab = ui.tab('Home')
        with ui.tab_panels(tabs, value=home_tab).classes('w-full'):
            with ui.tab_panel(home_tab):
                ui.button('Start BMS Monitoring', on_click=start_monitoring).classes('m-2')
                ui.button('Stop BMS Monitoring', on_click=stop_monitoring).classes('m-2')



ui.run(title='VoBLE', reload=True, host='0.0.0.0', port=4853, dark=True)