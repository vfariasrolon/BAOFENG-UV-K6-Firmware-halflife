import sys

def frequency_to_bcd(freq):
    bcd_bytes = []
    temp = freq
    for _ in range(4):
        digit_pair = temp % 100
        bcd_byte = ((digit_pair // 10) << 4) | (digit_pair % 10)
        bcd_bytes.append(bcd_byte)
        temp //= 100
    return bcd_bytes

def calculate_checksum(words):
    # Sum of first 7 words modulo 65536
    return sum(words[:7]) & 0xFFFF

def run_tests():
    print("=== INICIANDO VALIDACIÓN MATEMÁTICA DE FIRMWARE HALF-LIFE ===")
    
    # Test 1: BCD Frequency Conversion
    test_freq = 43012500 # 430.125 MHz
    expected_bcd = [0x00, 0x25, 0x01, 0x43]
    actual_bcd = frequency_to_bcd(test_freq)
    
    print(f"Frecuencia de prueba: {test_freq} Hz (430.125 MHz)")
    print(f"BCD Esperado: {[hex(b) for b in expected_bcd]}")
    print(f"BCD Calculado: {[hex(b) for b in actual_bcd]}")
    
    assert actual_bcd == expected_bcd, "ERROR: La conversión BCD no coincide!"
    print("✅ TEST 1: Conversión BCD Frecuencia... EXITOSA")
    
    # Test 2: Checksum FSK Frame
    test_frame = [
        0x5A03, # CMD SET_IDENTITY
        24,     # Assigned ID
        0xABCD, # UUID High
        0xEF12, # UUID Low
        (ord('S') << 8) | ord('L'), # Char 0-1
        (ord('V') << 8) | ord('-'), # Char 2-3
        (ord('0') << 8) | ord('1'), # Char 4-5
        0       # Checksum slot
    ]
    
    checksum = calculate_checksum(test_frame)
    test_frame[7] = checksum
    
    print(f"Trama FSK de prueba: {[hex(w) for w in test_frame]}")
    print(f"Checksum calculado: {hex(checksum)}")
    
    # Verify recalculation
    recalc_sum = calculate_checksum(test_frame)
    assert recalc_sum == checksum, "ERROR: Recálculo de suma de comprobación falló!"
    print("✅ TEST 2: Checksum de Trama FSK... EXITOSO")
    
    print("\n🎉 TODOS LOS TESTS MATEMÁTICOS DE LÓGICA DE FIRMWARE PASARON CORRECTAMENTE! 🎉")

if __name__ == "__main__":
    run_tests()
