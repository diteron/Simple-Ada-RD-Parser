function factorial (Num : Integer) return Integer
is
fact : Integer;
i : Integer;
begin
	fact := 1;
	i := 1;
	while  i <= Num loop
		fact := fact * i;
		i := i + 1;
	end loop;

    return fact;
end factorial;