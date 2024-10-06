#include "chip.hpp"

Chip::Chip() {
    memcpy(mem, fontset, 80);
}

int Chip::LoadRom(char *filepath) {
    std::string fp(filepath); 
    std::ifstream file(fp, std::ios::binary | std::ios::ate); 

    if (!file) {
        std::cerr << "Failed to load ROM file." << std::endl; 
        return LOADF;
    }

    std::streamsize fileSize = file.tellg(); 
    file.seekg(0, std::ios::beg);

    file.read(reinterpret_cast<char*>(&mem[512]), fileSize);
    if (!file) {
        std::cerr << "Failed to read ROM file." << std::endl;
        return READF;
    }

    file.close(); 
    return SUCCESS; 
}

int Chip::FetchDecodeExec() {
    OPRET res = SUCCESS; 

    uint16_t currIns = (mem[pc] << 8) | mem[pc + 1];
    
    std::cout << std::hex << static_cast<unsigned int>(currIns) << std::endl;
    pc += 2;

    uint8_t firstHex = (currIns >> 12) & 0xF;  
    uint8_t secondHex = (currIns >> 8) & 0xF;  
    uint8_t thirdHex = (currIns >> 4) & 0xF;  
    uint8_t fourthHex = currIns & 0xF;  


    switch(firstHex) {
        case 0: {
            if (thirdHex == 0xE && secondHex == 0) {
                if (fourthHex == 0) {
                    memset(vram, 0, sizeof(vram)); 
                    res = DRAW;
                }
                else if (fourthHex == 0xE) {
                    sp--;
                    pc = stack[sp];
                }
            } else { // ignored in modern interpreters 
                // uint16_t add = secondHex << 8 | thirdHex << 4 | fourthHex;
                // pc = add; 
            }
            break;
        }
        case 1: {
            uint16_t jpAdd = (secondHex << 8) | (thirdHex << 4) | fourthHex;
            pc = jpAdd; 
            break;
        }
        case 2: {
            uint16_t callAdd = (secondHex << 8) | (thirdHex << 4) | fourthHex;
            stack[sp] = pc; 
            sp++; 
            pc = callAdd; 
            break;
        }
        case 3: {
            uint8_t sameValue = (thirdHex << 4) | fourthHex;
            if (gen_regs[secondHex] == sameValue)
                pc += 2; 
            break;
        }
        case 4: {
            uint8_t diffValue = (thirdHex << 4) | fourthHex;
            if (gen_regs[secondHex] != diffValue)
                pc += 2;
            break;
        }
        case 5: {
            uint8_t reg = (thirdHex << 4) | fourthHex;
            if (gen_regs[secondHex] == gen_regs[reg])
                pc += 2;
            break;
        }
        case 6: {
            uint8_t ldValue = (thirdHex << 4) | fourthHex; 
            gen_regs[secondHex] = ldValue; 
            break;
        }
        case 7: {
            uint8_t addValue = (thirdHex << 4) | fourthHex; 
            gen_regs[secondHex] += addValue;
            break;
        }
        case 8: {
            switch(fourthHex) {
                case 0: {
                    gen_regs[secondHex] = gen_regs[thirdHex];
                    break; 
                }
                case 1: {
                    gen_regs[secondHex] = gen_regs[secondHex] | gen_regs[thirdHex]; 
                    break;
                }
                case 2: {
                    gen_regs[secondHex] = gen_regs[secondHex] & gen_regs[thirdHex]; 
                    break;
                }
                case 3: {
                    gen_regs[secondHex] = gen_regs[secondHex] ^ gen_regs[thirdHex]; 
                    break;
                }
                case 4: {
                    uint16_t res = gen_regs[secondHex] + gen_regs[thirdHex];
                    if (res > 255)
                        flag_reg = 1; 
                    else 
                        flag_reg = 0;
                    gen_regs[secondHex] = res;
                    break;             
                }
                case 5: {
                    if (gen_regs[secondHex] > gen_regs[thirdHex])
                        flag_reg = 1; 
                    else 
                        flag_reg = 0; 
                    gen_regs[secondHex] -= gen_regs[thirdHex]; 
                    break;
                }
                case 6: {
                    if ((gen_regs[secondHex] & 0x1) == 1)
                        flag_reg = 1; 
                    else 
                        flag_reg = 0;
                    gen_regs[secondHex] /= 2; 
                    break;
                }
                case 7: {
                    if (gen_regs[thirdHex] > gen_regs[secondHex])
                        flag_reg = 1; 
                    else 
                        flag_reg = 0; 
                    gen_regs[secondHex] = gen_regs[thirdHex] - gen_regs[secondHex]; 
                    break;
                }
                case 0xE: {
                    if ((gen_regs[secondHex] & 0x80) == 1) 
                        flag_reg = 1;
                    else 
                        flag_reg = 0; 
                    gen_regs[secondHex] *= 2; 
                    break;
                }
            }
            break;
        }
        case 9: {
            if (fourthHex == 0) {
                if (gen_regs[secondHex] != gen_regs[thirdHex])
                    pc += 2; 
            }
            break; 
        }
        case 0xA: {
            uint16_t ldAddress = (secondHex << 8) | (thirdHex << 4) | fourthHex; 
            mem_reg = ldAddress;
            break;
        }
        case 0xB: {
            uint16_t jpAddress = ((secondHex << 8) | (thirdHex << 4) | fourthHex) + gen_regs[0]; 
            pc = jpAddress; 
            break;
        }
        case 0xC: {
            std::random_device rd; 
            std::mt19937 generator(rd()); 
            std::uniform_int_distribution<int> distribution(0, 255); 

            uint8_t randomByte = static_cast<uint8_t>(distribution(generator));
            uint8_t andValue = (thirdHex << 4) | fourthHex; 
            
            gen_regs[secondHex] = randomByte & andValue; 
            break;
        }
        case 0xD: {
            uint8_t x = gen_regs[secondHex];
            uint8_t y = gen_regs[thirdHex];

            for (size_t i = 0; i < fourthHex; ++i) {
                for (int j = x; j < x + 8; ++j) {
                    uint8_t wrapJ = j % 64;  // makes sure the sprite wraps around

                    if (vram[y + i][wrapJ] ^ ((mem[mem_reg + i] >> (7 - (j - x))) & 1))
                        vram[y + i][wrapJ] = 1; 
                } 
            }
            res = DRAW;
        }
        case 0xE: {
            if (thirdHex == 9 && fourthHex == 0xE) {
                if (keys[gen_regs[secondHex]]) // checks if the key at the index of the value of the register is pressed
                    pc += 2; 
            }
            else if (thirdHex == 0xA && fourthHex == 1)
                if (!keys[gen_regs[secondHex]])
                    pc += 2; 
            break;
        }
        case 0xF: {
            if (thirdHex == 0 && fourthHex == 7)
                gen_regs[secondHex] = delay_reg; 
            else if (thirdHex == 0 && fourthHex == 0xA) {
                for (int i = 0; i < 16; ++i)
                    if (keys[i]) {
                        gen_regs[secondHex] = i;         
                        return SUCCESS; 
                    }
                pc -= 2; 
                return SLEEP; 
            }
            else if (thirdHex == 1 && fourthHex == 5) {
                delay_reg = gen_regs[secondHex]; 
            }
            else if (thirdHex == 1 && fourthHex == 8) {
                sound_reg = gen_regs[secondHex]; 
            }
            else if (thirdHex == 1 && fourthHex == 0xE) {
                mem_reg += gen_regs[secondHex]; 
            }
            else if (thirdHex == 2 && fourthHex == 9) {
                uint8_t ind = gen_regs[secondHex]; 
                mem_reg = ind * 5;
            } 
            else if (thirdHex == 3 && fourthHex == 3) {
                uint8_t h = gen_regs[secondHex] / 100;
                uint8_t t = (gen_regs[secondHex] / 10) % 10; 
                uint8_t u = gen_regs[secondHex] % 10; 
                mem[mem_reg] = h; 
                mem[mem_reg + 1] = t; 
                mem[mem_reg + 2] = u; 
            }
            else if (thirdHex == 5 && fourthHex == 5) {
                if (secondHex == 15) {
                    mem[mem_reg + 15] = flag_reg; 
                } else {
                    for (size_t i = 0; i <= secondHex; ++i) {
                        mem[mem_reg + i] = gen_regs[i]; 
                    }
                }
            }
            else if (thirdHex == 6 && fourthHex == 5) {
                if (secondHex == 15) {
                    flag_reg = mem[mem_reg + 15]; 
                } else {
                    for (int i = 0; i <= secondHex; ++i) {
                        gen_regs[i] = mem[mem_reg + i]; 
                    }
                }
            }
        }
    }
    return res;
} 
uint8_t (&Chip::getVram())[32][64] {
    return vram;
}         
