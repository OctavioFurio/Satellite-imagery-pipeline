import numpy as np

# Helpers

def read_alist(path):
    with open(path) as f:
        N, M = map(int, f.readline().split())
        _, max_row_weight = map(int, f.readline().split())
        f.readline()  # col weights
        f.readline()  # row weights
        for _ in range(N): f.readline()
        H = np.zeros((M, N), dtype=bool)
        for i in range(M):
            row = f.readline().split()
            for x in row[:max_row_weight]:
                idx = int(x)
                if idx != 0:
                    H[i, idx - 1] = True
    return H, N, M


def awgn_channel(codeword, snr_db):
    snr_linear = 10 ** (snr_db / 10)
    sigma = np.sqrt(1 / (2 * snr_linear))
    
    bpsk = 1 - 2 * codeword.astype(np.float32)  # 0/1 -> 1/-1
    noisy = bpsk + np.random.normal(0, sigma, len(bpsk))
    
    return (2 * noisy / sigma**2).astype(np.float32)


# LDPC

class LDPCEncoder:
    def __init__(self, alist_path, X, D):
        self.H, self.N, self.M = read_alist(alist_path)
        self.X, self.D = X, D
        self.Z = self.N // 24
        self.H_sys = self.H[:, :self.N - self.M]

    def _compute_parity(self, V):
        M, Z, X = self.M, self.Z, self.X
        p = np.zeros(M, dtype=np.uint8)

        for i in range(Z, (X + 1) * Z):
            p[i] = V[i - Z] ^ p[i - Z]

        for i in range(M - 1, M - 1 - Z, -1):
            p[i] = V[i] ^ p[(M - 1) - i]

        for i in range(M - 1 - Z, (X + 1) * Z - 1, -1):
            p[i] = V[i] ^ p[i + Z]

        p0 = np.array([V[X*Z + k] ^ p[Z + X*Z + k] for k in range(Z)], dtype=np.uint8)
        p[:Z] = p0 ^ p[X*Z : X*Z + Z]

        d = int(np.floor(self.D * self.Z / 96)) if self.D > 0 else self.D
        correction = np.roll(p[:Z], -d)
        p[Z:] ^= correction[np.arange(M - Z) % Z]

        return p

    def encode(self, info_bits):
        V = (self.H_sys @ info_bits.astype(np.int32)) % 2
        return np.concatenate([info_bits, self._compute_parity(V.astype(np.uint8))]).astype(np.uint8)

    def check(self, codeword):
        return ((self.H @ codeword.astype(np.int32)) % 2).sum() == 0


class LDPCDecoder:
    def __init__(self, alist_path):
        self.H, self.N, self.M = read_alist(alist_path)
        self.check_to_var = [np.where(self.H[m])[0] for m in range(self.M)]

    def decode(self, received, max_iter=50):
        llr = (np.where(received == 0, 1.0, -1.0).astype(np.float32)
               if received.dtype != np.float32 else received.copy())

        max_deg = max(len(nb) for nb in self.check_to_var)
        messages = np.zeros((self.M, max_deg), dtype=np.float32)

        for _ in range(max_iter):
            for m in range(self.M):
                nb = self.check_to_var[m]
                d  = len(nb)

                incoming = llr[nb] - messages[m, :d]

                sign = np.float32(-1) if (incoming < 0).sum() % 2 else np.float32(1)
                mag  = np.abs(incoming)
                m1, m2 = np.partition(mag, 1)[:2] if d > 1 else (mag[0], mag[0])

                out_mag  = np.where((mag == m1) & (m1 != m2), m2, m1)
                out_sign = np.where(incoming < 0, -sign, sign)

                messages[m, :d] = out_sign * out_mag
                llr[nb] = incoming + messages[m, :d]

            if not ((self.H @ (llr <= 0).astype(np.uint8)) % 2).any():
                break

        return (llr <= 0).astype(np.uint8)

    def syndrome_weight(self, codeword):
        return int(((self.H @ codeword.astype(np.int32)) % 2).sum())