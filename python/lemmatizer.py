import sys
import spacy
from collections import Counter


# Load spaCy model # I HATE THIS FUCKING SHIT
try:
    nlp = spacy.load("en_core_web_sm")
except OSError:
    print("no spacy", file=sys.stderr)
    sys.exit(1)

# Medical/biomedical terms to preserve (keep original form) # AI gen
MEDICAL_PRESERVE = {
    'covid', 'covid-19', 'covid19', 'sars', 'sars-cov', 'sars-cov-2', 
    'mers', 'mers-cov', 'coronavirus', 'coronaviruses',
    'antibody', 'antibodies', 'antigen', 'antigens',
    'vaccine', 'vaccines', 'vaccination',
    'virus', 'viruses', 'viral', 'virion', 'virions',
    'protein', 'proteins', 'peptide', 'peptides',
    'rna', 'dna', 'mrna', 'trna', 'rrna',
    'genome', 'genomic', 'gene', 'genes',
    'cell', 'cells', 'cellular', 'cytokine', 'cytokines',
    'pneumonia', 'influenza', 'respiratory', 'pulmonary',
    'syndrome', 'disease', 'infection', 'infectious',
    'transmission', 'transmissible', 'contagious',
    'symptom', 'symptoms', 'asymptomatic', 'symptomatic',
    'diagnosis', 'diagnostic', 'prognosis', 'prognostic',
    'treatment', 'therapeutic', 'therapy', 'therapies',
    'immune', 'immunity', 'immunology', 'immunological',
    'pathogen', 'pathogens', 'pathogenic', 'pathogenesis',
    'pandemic', 'epidemic', 'endemic', 'outbreak',
    'mortality', 'morbidity', 'fatality',
    'patient', 'patients', 'clinical', 'hospital',
    'icu', 'intensive', 'ventilator', 'ventilation',
    'oxygen', 'hypoxia', 'hypoxic',
    'inflammation', 'inflammatory',
    'receptor', 'receptors', 'ace2',
    'spike', 'nucleocapsid', 'membrane',
    'antibacterial', 'antiviral', 'antimicrobial'
}

# Medical abbreviations (always preserve)
MEDICAL_ABBREV = {
    'who', 'cdc', 'fda', 'nih', 'niaid', 'ema',
    'pcr', 'rt-pcr', 'qrt-pcr', 'elisa',
    'ards', 'icu', 'ecmo',
    'il', 'il-6', 'il-1', 'tnf', 'tnf-alpha', 'ifn',
    'igg', 'igm', 'iga', 'ige',
    'hiv', 'aids', 'tb', 'ebv', 'cmv',
    'ct', 'mri', 'xray', 'ecg', 'ekg',
    'mg', 'ml', 'kg', 'mcg', 'ng', 'pg',
    'usa', 'uk', 'eu', 'china', 'cdc'
}

def lemmatize_text(text):
        # process with spaCy
    doc = nlp(text)
    
    lexicon = Counter()
    
    for token in doc:
        # Skip punctuation, spaces, and numbers
        if token.is_punct or token.is_space or token.like_num:
            continue
        
        # get lowercase version for checking
        token_lower = token.text.lower()
        
        # skip very short tokens (unless medical abbreviation)
        if len(token_lower) < 2:
            continue
        
        if len(token_lower) == 2 and token_lower not in MEDICAL_ABBREV:
            continue
        
        # skip common stop words (but not medical ones)
        if token.is_stop and token_lower not in MEDICAL_PRESERVE:
            continue
        
        # preserve medical terms and abbreviations as-is
        if token_lower in MEDICAL_PRESERVE or token_lower in MEDICAL_ABBREV:
            lexicon[token_lower] += 1
            continue
        
        # get lemma from spaCy
        lemma = token.lemma_.lower().strip()
        
        # skip if lemma is empty or too short
        if not lemma or len(lemma) < 3:
            continue
        
        # skip common pronouns and determiners
        if lemma in {'i', 'he', 'she', 'it', 'we', 'they', 'me', 'him', 
                     'her', 'us', 'them', 'my', 'his', 'her', 'its', 
                     'our', 'their', 'this', 'that', 'these', 'those'}:
            continue
        
        lexicon[lemma] += 1
    
    return lexicon

def main():
    # read all text from stdin
    text = sys.stdin.read()
    
    if not text.strip():
        print("ERROR: No input text received", file=sys.stderr)
        sys.exit(1)
    
    # process text
    print("Processing text...", file=sys.stderr)
    lexicon = lemmatize_text(text)
    
    # output CSV format to stdout
    print("word,frequency")
    for word, freq in lexicon.most_common():
        # Escape any commas (shouldn't happen but be safe)
        word_escaped = word.replace(',', '_')
        print(f"{word_escaped},{freq}")
    
    # log statistics to stderr (won't interfere with CSV output)
    print(f"Processed {len(lexicon)} unique terms", file=sys.stderr)
    
    # show top 10 for debugging
    print("Top 10 terms:", file=sys.stderr)
    for i, (word, freq) in enumerate(lexicon.most_common(10), 1):
        print(f"  {i}. {word}: {freq}", file=sys.stderr)

if __name__ == "__main__":
    main()


