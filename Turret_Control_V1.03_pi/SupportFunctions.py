def RemapValue(temp_input, temp_old_min, temp_old_max, temp_new_min, temp_new_max):
    NewValue = (((temp_input - temp_old_min) * (temp_new_max - temp_new_min)) / (temp_old_max - temp_old_min)) + temp_new_min 
    return NewValue
