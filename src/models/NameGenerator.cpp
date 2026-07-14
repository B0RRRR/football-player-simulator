#include "NameGenerator.h"
#include <vector>
#include <cstdlib>

struct NamePool {
    std::vector<std::string> firstNames;
    std::vector<std::string> lastNames;
};

static NamePool getPoolForNation(const std::string& nationality) {
    if (nationality == "England") {
        return {
            {"Harry", "Jack", "Oliver", "Charlie", "Jacob", "Thomas", "George", "James", "William", "Marcus", "Phil", "Mason", "Declan", "Jordan", "Bukayo"},
            {"Smith", "Jones", "Taylor", "Williams", "Brown", "Davies", "Evans", "Wilson", "Thomas", "Roberts", "Kane", "Foden", "Mount", "Rice", "Saka"}
        };
    } else if (nationality == "Spain") {
        return {
            {"Pablo", "Juan", "Carlos", "Jose", "Manuel", "Luis", "David", "Javier", "Daniel", "Sergio", "Pedri", "Gavi", "Ferran", "Ansu", "Marco"},
            {"Garcia", "Martinez", "Rodriguez", "Fernandez", "Lopez", "Gonzalez", "Perez", "Sanchez", "Romero", "Torres", "Ramos", "Fati", "Asensio"}
        };
    } else if (nationality == "Italy") {
        return {
            {"Francesco", "Alessandro", "Lorenzo", "Leonardo", "Mattia", "Matteo", "Gabriele", "Riccardo", "Federico", "Ciro", "Nicolo", "Gianluigi", "Marco"},
            {"Rossi", "Russo", "Ferrari", "Esposito", "Bianchi", "Romano", "Colombo", "Ricci", "Marino", "Greco", "Barella", "Donnarumma", "Immobile"}
        };
    } else if (nationality == "Germany") {
        return {
            {"Lukas", "Leon", "Finn", "Paul", "Jonas", "Ben", "Elias", "Felix", "Maximilian", "Toni", "Thomas", "Kai", "Leroy", "Jamal", "Joshua"},
            {"Muller", "Schmidt", "Schneider", "Fischer", "Weber", "Meyer", "Wagner", "Becker", "Hoffmann", "Schulz", "Kroos", "Havertz", "Sane", "Musiala"}
        };
    } else if (nationality == "France") {
        return {
            {"Kylian", "Antoine", "Paul", "N'Golo", "Hugo", "Olivier", "Karim", "Aurelien", "Ousmane", "Eduardo", "Jules", "Raphael", "Lucas", "Theo", "Dayot"},
            {"Mbappe", "Griezmann", "Pogba", "Kante", "Lloris", "Giroud", "Benzema", "Tchouameni", "Dembele", "Camavinga", "Kounde", "Varane", "Hernandez"}
        };
    } else if (nationality == "Portugal") {
        return {
            {"Cristiano", "Bruno", "Bernardo", "Ruben", "Joao", "Diogo", "Rafael", "Nuno", "Pepe", "Rui", "Goncalo", "Andre", "Vitinha", "Renato"},
            {"Ronaldo", "Fernandes", "Silva", "Dias", "Cancelo", "Jota", "Leao", "Mendes", "Patricio", "Felix", "Gomes", "Sanches", "Guerreiro"}
        };
    } else if (nationality == "Brazil") {
        return {
            {"Neymar", "Vinicius", "Rodrygo", "Casemiro", "Alisson", "Ederson", "Thiago", "Marquinhos", "Eder", "Gabriel", "Antony", "Raphinha", "Richarlison"},
            {"Junior", "Silva", "Militão", "Jesus", "Martinelli", "Guimaraes", "Paqueta", "Fred", "Fabinho", "Coutinho", "Firmino", "Ribeiro", "Pedro"}
        };
    } else if (nationality == "Argentina") {
        return {
            {"Lionel", "Angel", "Emiliano", "Rodrigo", "Enzo", "Julian", "Lautaro", "Lisandro", "Cristian", "Nicolas", "Alexis", "Paulo", "Gonzalo"},
            {"Messi", "Di Maria", "Martinez", "De Paul", "Fernandez", "Alvarez", "Romero", "Otamendi", "Tagliafico", "Mac Allister", "Dybala", "Montiel"}
        };
    } else if (nationality == "Netherlands") {
        return {
            {"Virgil", "Frenkie", "Memphis", "Matthijs", "Cody", "Denzel", "Nathan", "Jurrien", "Steven", "Teun", "Xavi", "Daley", "Stefan"},
            {"van Dijk", "de Jong", "Depay", "de Ligt", "Gakpo", "Dumfries", "Ake", "Timber", "Bergwijn", "Koopmeiners", "Simons", "Blind", "de Vrij"}
        };
    } else {
        // Fallback for other nations / generic
        return {
            {"Alex", "John", "Max", "David", "Lucas", "Tom", "Ben", "Sam", "Daniel", "Kevin"},
            {"Smith", "Muller", "Silva", "Ivanov", "Kim", "Olsen", "Andersson", "Kowalski", "Novak"}
        };
    }
}

std::string NameGenerator::generateName(const std::string& nationality) {
    NamePool pool = getPoolForNation(nationality);
    std::string first = pool.firstNames[rand() % pool.firstNames.size()];
    std::string last = pool.lastNames[rand() % pool.lastNames.size()];
    return first + " " + last;
}
