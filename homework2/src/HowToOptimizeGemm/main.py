import pandas as pd


data_string = """The total instructions executed are 4641045785, total cycles 1477936592

The total instructions executed are 15528713051, total cycles 4975627818

The total instructions executed are 36649423819, total cycles 12257851147

The total instructions executed are 71391458959, total cycles 25397305447

The total instructions executed are 123146618940, total cycles 46094089792
"""

name = 'ge8'


instructions = {}
cycles = {}
words = data_string.split()
i = 500
for word in words:
    if word[0] in ['0','1','2','3','4','5','6','7','8','9']:
        if word[-1] == ',':
            word = word[0 : len(word) - 1]
            instructions[i] = int(word)
        else:
            cycles[i] = int(word)
            i += 500

# df = pd.DataFrame(list(instructions.items()), columns=['Rozmiar', 'chol1'])
# df.to_excel('instrukcje.xlsx', index=False)
#
# df2 = pd.DataFrame(list(cycles.items()), columns=['Rozmiar', 'chol1'])
# df2.to_excel('cykle.xlsx', index=False)

df = pd.read_excel('instrukcje.xlsx')
df[name] = list(instructions.values())
df.to_excel('instrukcje.xlsx', index=False)

df2 = pd.read_excel('cykle.xlsx')
df2[name] = list(cycles.values())
df2.to_excel('cykle.xlsx', index=False)


print("Dane zostały zapisane do pliku.")


print(instructions)
