from nicegui import ui
import asyncio

from bmslib.jikong import JKBt

async def main():
    mac_address = 'C8:47:80:23:4F:95'  # Replace with your BMS MAC address

    bms = JKBt(mac_address, name='jk', verbose_log=False)
    async with bms:
        while True:
            try:
                s = await bms.fetch(wait=True)
                print(f"SOC: {repr(s.soc)} Current: {s.current:.3f} Voltage: {s.voltage:.3f} Temp: {s.temperatures} I_bal: {s.balance_current} Voltages: {await bms.fetch_voltages()}")
            except KeyboardInterrupt:
                break


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
                ui.button('Start BMS Monitoring', on_click=lambda: asyncio.create_task(main())).classes('m-2')



ui.run(title='VoBLE', reload=True, host='0.0.0.0', port=12000, dark=True)