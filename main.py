from bmslib.jikong import JKBt

async def main():
    mac_address = 'C8:47:80:23:4F:95'  # Replace with your BMS MAC address

    bms = JKBt(mac_address, name='jk', verbose_log=False)
    async with bms:
        while True:
            try:
                s = await bms.fetch(wait=True)
                # print(s, 'I_bal=', s.balance_current, await bms.fetch_voltages())
                print(f"SOC: {repr(s.soc)} Current: {s.current:.3f} Voltage: {s.voltage:.3f} Temp: {s.temperatures} I_bal: {s.balance_current} Voltages: {await bms.fetch_voltages()}")
            except KeyboardInterrupt:
                break


if __name__ == '__main__':
    import asyncio
    asyncio.run(main())