from bmslib.jikong import JKBt

async def main():
    mac_address = 'C8:47:80:23:4F:95'  # caravan (intel)

    bms = JKBt(mac_address, name='jk', verbose_log=False)
    async with bms:
        while True:
            try:
                s = await bms.fetch(wait=True)
                # print(s, 'I_bal=', s.balance_current, await bms.fetch_voltages())
                print(f"SOC: {s.soc} Current: {s.current} Voltage: {s.voltage} Temp: {s.temperatures} I_bal: {s.balance_current}")
            except KeyboardInterrupt:
                break


if __name__ == '__main__':
    import asyncio
    asyncio.run(main())